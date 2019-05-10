// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: Page.cpp
// Description: Page Class
//      Author: Ziming Li
//     Created: 2019-02-12
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All Rights Reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "core/page/Page.h"

#include "core/css/resolver/ViewportStyleResolver.h"
#include "core/dom/ClientRectList.h"
#include "core/dom/VisitedLinkState.h"
#include "core/editing/DragCaretController.h"
#include "core/editing/commands/UndoStack.h"
#include "core/editing/markers/DocumentMarkerController.h"
#include "core/events/Event.h"
#include "core/fetch/MemoryCache.h"
#include "core/fetch/ResourceFetcher.h"
#include "core/frame/FrameConsole.h"
#include "core/frame/FrameHost.h"
#include "core/frame/FrameView.h"
#include "core/frame/Settings.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/layout/LayoutView.h"
#include "core/layout/TextAutosizer.h"
#include "core/page/AutoscrollController.h"
#include "core/page/ChromeClient.h"
#include "core/page/ContextMenuController.h"
#include "core/page/DragController.h"
#include "core/page/FocusController.h"
#include "core/page/PointerLockController.h"
#include "core/page/ValidationMessageClient.h"
#include "core/page/scrolling/ScrollingCoordinator.h"
#include "core/paint/PaintLayer.h"
#include "platform/UserGestureIndicator.h"
#include "platform/graphics/GraphicsLayer.h"
#include "public/platform/Platform.h"

namespace blink {

// Set of all live pages; includes internal Page objects that are
// not observable from scripts.
static Page::PageSet& allPages()
{
    DEFINE_STATIC_LOCAL(Page::PageSet, allPages, ());
    return allPages;
}

Page::PageSet& Page::ordinaryPages()
{
    DEFINE_STATIC_LOCAL(Page::PageSet, ordinaryPages, ());
    return ordinaryPages;
}

void Page::networkStateChanged(bool online)
{
    WillBeHeapVector<RefPtrWillBeMember<LocalFrame>> frames;

    // Get all the frames of all the pages in all the page groups
    for (Page* page : allPages()) {
        frames.append(toLocalFrame(page->mainFrame()));
    }

    AtomicString eventName = online ? EventTypeNames::online : EventTypeNames::offline;
    for (const auto& frame : frames) {
        frame->domWindow()->dispatchEvent(Event::create(eventName));
        InspectorInstrumentation::networkStateChanged(frame.get(), online);
    }
}

void Page::onMemoryPressure()
{
    assert(false); // Low memory!
}

float deviceScaleFactor(LocalFrame* frame)
{
    if (!frame)
        return 1;
    Page* page = frame->page();
    if (!page)
        return 1;
    return page->deviceScaleFactor();
}

PassOwnPtrWillBeRawPtr<Page> Page::createOrdinary(PageClients& pageClients)
{
    OwnPtrWillBeRawPtr<Page> page = create(pageClients);
    ordinaryPages().add(page.get());
    return page.release();
}

Page::Page(PageClients& pageClients)
    : SettingsDelegate(Settings::create())
    , m_animator(PageAnimator::create(*this))
    , m_autoscrollController(AutoscrollController::create(*this))
    , m_chromeClient(pageClients.chromeClient)
    , m_dragCaretController(DragCaretController::create())
    , m_dragController(DragController::create(this, pageClients.dragClient))
    , m_focusController(FocusController::create(this))
    , m_contextMenuController(ContextMenuController::create(this, pageClients.contextMenuClient))
    , m_pointerLockController(PointerLockController::create(this))
    , m_undoStack(UndoStack::create())
    , m_mainFrame(nullptr)
    , m_editorClient(pageClients.editorClient)
    , m_openedByDOM(false)
    , m_tabKeyCyclesThroughElements(true)
    , m_defersLoading(false)
    , m_deviceScaleFactor(1)
    , m_visibilityState(PageVisibilityStateVisible)
    , m_isCursorVisible(true)
#if ENABLE(ASSERT)
    , m_isPainting(false)
#endif
    , m_frameHost(FrameHost::create(*this))
{
    ASSERT(m_editorClient);

    ASSERT(!allPages().contains(this));
    allPages().add(this);
}

Page::~Page()
{
#if !ENABLE(OILPAN)
    ASSERT(!ordinaryPages().contains(this));
#endif
    // willBeDestroyed() must be called before Page destruction.
    ASSERT(!m_mainFrame);
}

ViewportDescription Page::viewportDescription() const
{
    return mainFrame() && mainFrame()->isLocalFrame() && deprecatedLocalMainFrame()->document() ? deprecatedLocalMainFrame()->document()->viewportDescription() : ViewportDescription();
}

ScrollingCoordinator* Page::scrollingCoordinator()
{
    if (!m_scrollingCoordinator && m_settings->acceleratedCompositingEnabled())
        m_scrollingCoordinator = ScrollingCoordinator::create(this);

    return m_scrollingCoordinator.get();
}

String Page::mainThreadScrollingReasonsAsText()
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->mainThreadScrollingReasonsAsText();

    return String();
}

ClientRectList* Page::nonFastScrollableRects(const LocalFrame* frame)
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator()) {
        // Hits in compositing/iframes/iframe-composited-scrolling.html
        DisableCompositingQueryAsserts disabler;
        scrollingCoordinator->updateAfterCompositingChangeIfNeeded();
    }

    if (!frame->view()->layerForScrolling())
        return ClientRectList::create();

    // Now retain non-fast scrollable regions
    return ClientRectList::create(frame->view()->layerForScrolling()->platformLayer()->nonFastScrollableRegion());
}

void Page::setMainFrame(Frame* mainFrame)
{
    // Should only be called during initialization or swaps between local and
    // remote frames.
    // FIXME: Unfortunately we can't assert on this at the moment, because this
    // is called in the base constructor for both LocalFrame and RemoteFrame,
    // when the vtables for the derived classes have not yet been setup.
    m_mainFrame = mainFrame;
}

void Page::documentDetached(Document* document)
{
    m_multisamplingChangedObservers.clear();
    m_pointerLockController->documentDetached(document);
    m_contextMenuController->documentDetached(document);
    if (m_validationMessageClient)
        m_validationMessageClient->documentDetached(*document);
    m_originsUsingFeatures.documentDetached(*document);
}

bool Page::openedByDOM() const
{
    return m_openedByDOM;
}

void Page::setOpenedByDOM()
{
    m_openedByDOM = true;
}

void Page::platformColorsChanged()
{
    for (const Page* page : allPages())
        toLocalFrame(page->mainFrame())->document()->platformColorsChanged();
}

void Page::setNeedsRecalcStyleInAllFrames()
{
    toLocalFrame(mainFrame())->document()->styleResolverChanged();
}

void Page::setNeedsLayoutInAllFrames()
{
    if (FrameView *view = toLocalFrame(mainFrame())->view())
    {
        view->setNeedsLayout();
        view->scheduleRelayout();
    }
}

void Page::unmarkAllTextMatches()
{
    toLocalFrame(mainFrame())->document()->markers().removeMarkers(DocumentMarker::TextMatch);
}

void Page::setValidationMessageClient(PassOwnPtrWillBeRawPtr<ValidationMessageClient> client)
{
    m_validationMessageClient = client;
}

void Page::setDefersLoading(bool defers)
{
    if (defers == m_defersLoading)
        return;

    m_defersLoading = defers;
    toLocalFrame(mainFrame())->loader().setDefersLoading(defers);
}

void Page::setPageScaleFactor(float scale)
{
    frameHost().visualViewport().setScale(scale);
}

float Page::pageScaleFactor() const
{
    return frameHost().visualViewport().scale();
}

void Page::setDeviceScaleFactor(float scaleFactor)
{
    if (m_deviceScaleFactor == scaleFactor)
        return;

    m_deviceScaleFactor = scaleFactor;
    setNeedsRecalcStyleInAllFrames();

    if (mainFrame() && mainFrame()->isLocalFrame())
        deprecatedLocalMainFrame()->deviceScaleFactorChanged();
}

void Page::setDeviceColorProfile(const Vector<char>& profile)
{
    // FIXME: implement.
}

void Page::resetDeviceColorProfileForTesting()
{
    assert(false); // Not reached!
}

void Page::allVisitedStateChanged(bool invalidateVisitedLinkHashes)
{
    for (const Page* page : ordinaryPages())
        toLocalFrame(page->m_mainFrame)->document()->visitedLinkState().invalidateStyleForAllLinks(invalidateVisitedLinkHashes);
}

void Page::visitedStateChanged(LinkHash linkHash)
{
    for (const Page* page : ordinaryPages())
        toLocalFrame(page->m_mainFrame)->document()->visitedLinkState().invalidateStyleForLink(linkHash);
}

void Page::setVisibilityState(PageVisibilityState visibilityState, bool isInitialState)
{
    if (m_visibilityState == visibilityState)
        return;
    m_visibilityState = visibilityState;

    if (!isInitialState)
        notifyPageVisibilityChanged();

    if (!isInitialState && m_mainFrame && m_mainFrame->isLocalFrame())
        deprecatedLocalMainFrame()->didChangeVisibilityState();
}

PageVisibilityState Page::visibilityState() const
{
    return m_visibilityState;
}

bool Page::isCursorVisible() const
{
    return m_isCursorVisible && settings().deviceSupportsMouse();
}

void Page::addMultisamplingChangedObserver(MultisamplingChangedObserver* observer)
{
    m_multisamplingChangedObservers.add(observer);
}

// For Oilpan, unregistration is handled by the GC and weak references.
#if !ENABLE(OILPAN)
void Page::removeMultisamplingChangedObserver(MultisamplingChangedObserver* observer)
{
    m_multisamplingChangedObservers.remove(observer);
}
#endif

void Page::settingsChanged(SettingsDelegate::ChangeType changeType)
{
    switch (changeType) {
    case SettingsDelegate::StyleChange:
        setNeedsRecalcStyleInAllFrames();
        break;
    case SettingsDelegate::ViewportDescriptionChange:
        if (mainFrame() && mainFrame()->isLocalFrame())
            deprecatedLocalMainFrame()->document()->updateViewportDescription();
        break;
    case SettingsDelegate::DNSPrefetchingChange:
        ASSERT_NOT_REACHED();
        break;
    case SettingsDelegate::MultisamplingChange: {
        for (MultisamplingChangedObserver* observer : m_multisamplingChangedObservers)
            observer->multisamplingChanged(m_settings->openGLMultisamplingEnabled());
        break;
    }
    case SettingsDelegate::ImageLoadingChange: {
        ResourceFetcher *fetcher = toLocalFrame(mainFrame())->document()->fetcher();
        fetcher->setImagesEnabled(settings().imagesEnabled());
        fetcher->setAutoLoadImages(settings().loadsImagesAutomatically());
        break;
    }
    case SettingsDelegate::TextAutosizingChange:
        if (!mainFrame() || !mainFrame()->isLocalFrame())
            break;
        if (TextAutosizer* textAutosizer = deprecatedLocalMainFrame()->document()->textAutosizer())
            textAutosizer->updatePageInfoInAllFrames();
        break;
    case SettingsDelegate::FontFamilyChange:
        toLocalFrame(mainFrame())->document()->styleEngine().updateGenericFontFamilySettings();
        setNeedsRecalcStyleInAllFrames();
        break;
    case SettingsDelegate::AcceleratedCompositingChange:
        updateAcceleratedCompositingSettings();
        break;
    case SettingsDelegate::MediaQueryChange:
        toLocalFrame(mainFrame())->document()->mediaQueryAffectingValueChanged();
        break;
    case SettingsDelegate::AccessibilityStateChange:
        // Nothing to do.
        break;
    case SettingsDelegate::ViewportRuleChange:
        {
            if (!mainFrame() || !mainFrame()->isLocalFrame())
                break;
            Document* doc = toLocalFrame(mainFrame())->document();
            if (!doc || !doc->styleResolver())
                break;
            doc->styleResolver()->viewportStyleResolver()->collectViewportRules();
        }
        break;
    }
}

void Page::updateAcceleratedCompositingSettings()
{
    if (FrameView *view = toLocalFrame(mainFrame())->view())
        view->updateAcceleratedCompositingSettings();
}

void Page::didCommitLoad(LocalFrame* frame)
{
    notifyDidCommitLoad(frame);
    if (m_mainFrame == frame) {
        frame->console().clearMessages();
        useCounter().didCommitLoad();
        frameHost().visualViewport().sendUMAMetrics();
        m_originsUsingFeatures.updateMeasurementsAndClear();
        UserGestureIndicator::clearProcessedUserGestureSinceLoad();
    }
}

void Page::acceptLanguagesChanged()
{
    toLocalFrame(mainFrame())->localDOMWindow()->acceptLanguagesChanged();
}

DEFINE_TRACE(Page)
{
#if ENABLE(OILPAN)
    visitor->trace(m_animator);
    visitor->trace(m_autoscrollController);
    visitor->trace(m_chromeClient);
    visitor->trace(m_dragCaretController);
    visitor->trace(m_dragController);
    visitor->trace(m_focusController);
    visitor->trace(m_contextMenuController);
    visitor->trace(m_pointerLockController);
    visitor->trace(m_scrollingCoordinator);
    visitor->trace(m_undoStack);
    visitor->trace(m_mainFrame);
    visitor->trace(m_validationMessageClient);
    visitor->trace(m_multisamplingChangedObservers);
    visitor->trace(m_frameHost);
    HeapSupplementable<Page>::trace(visitor);
#endif
    PageLifecycleNotifier::trace(visitor);
}

void Page::layerTreeViewInitialized(WebLayerTreeView& layerTreeView)
{
    if (scrollingCoordinator())
        scrollingCoordinator()->layerTreeViewInitialized(layerTreeView);
}

void Page::willCloseLayerTreeView(WebLayerTreeView& layerTreeView)
{
    if (m_scrollingCoordinator)
        m_scrollingCoordinator->willCloseLayerTreeView(layerTreeView);
}

void Page::willBeClosed()
{
    ordinaryPages().remove(this);
}

void Page::willBeDestroyed()
{
    RefPtrWillBeRawPtr<Frame> mainFrame = m_mainFrame;

    mainFrame->detach(FrameDetachType::Remove);

    ASSERT(allPages().contains(this));
    allPages().remove(this);
    ordinaryPages().remove(this);

    if (m_scrollingCoordinator)
        m_scrollingCoordinator->willBeDestroyed();

    chromeClient().chromeDestroyed();
    if (m_validationMessageClient)
        m_validationMessageClient->willBeDestroyed();
    m_mainFrame = nullptr;

    PageLifecycleNotifier::notifyContextDestroyed();
}

Page::PageClients::PageClients()
    : chromeClient(nullptr)
    , contextMenuClient(nullptr)
    , editorClient(nullptr)
    , dragClient(nullptr)
{
}

Page::PageClients::~PageClients()
{
}

template class CORE_TEMPLATE_EXPORT WillBeHeapSupplement<Page>;

} // namespace blink
