// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: WebLocalFrameImpl.cpp
// Description: WebLocalFrameImpl Class
//      Author: Ziming Li
//     Created: 2019-03-05
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// How ownership works
// -------------------
//
// Big oh represents a refcounted relationship: owner O--- ownee
//
// WebView (for the toplevel frame only)
//    O
//    |           WebFrame
//    |              O
//    |              |
//   Page O------- LocalFrame (m_mainFrame) O-------O FrameView
//                   ||
//                   ||
//               FrameLoader
//
// FrameLoader and LocalFrame are formerly one object that was split apart because
// it got too big. They basically have the same lifetime, hence the double line.
//
// From the perspective of the embedder, WebFrame is simply an object that it
// allocates by calling WebFrame::create() and must be freed by calling close().
// Internally, WebFrame is actually refcounted and it holds a reference to its
// corresponding LocalFrame in blink.
//
// Oilpan: the middle objects + Page in the above diagram are Oilpan heap allocated,
// WebView and FrameView are currently not. In terms of ownership and control, the
// relationships stays the same, but the references from the off-heap WebView to the
// on-heap Page is handled by a Persistent<>, not a RefPtr<>. Similarly, the mutual
// strong references between the on-heap LocalFrame and the off-heap FrameView
// is through a RefPtr (from LocalFrame to FrameView), and a Persistent refers
// to the LocalFrame in the other direction.
//
// From the embedder's point of view, the use of Oilpan brings no changes. close()
// must still be used to signal that the embedder is through with the WebFrame.
// Calling it will bring about the release and finalization of the frame object,
// and everything underneath.
//
// How frames are destroyed
// ------------------------
//
// The main frame is never destroyed and is re-used. The FrameLoader is re-used
// and a reference to the main frame is kept by the Page.
//
// When frame content is replaced, all subframes are destroyed. This happens
// in Frame::detachChildren for each subframe in a pre-order depth-first
// traversal. Note that child node order may not match DOM node order!
// detachChildren() (virtually) calls Frame::detach(), which again calls
// FrameLoaderClient::detached(). This triggers WebFrame to clear its reference to
// LocalFrame. FrameLoaderClient::detached() also notifies the embedder via WebFrameClient
// that the frame is detached. Most embedders will invoke close() on the WebFrame
// at this point, triggering its deletion unless something else is still retaining a reference.
//
// The client is expected to be set whenever the WebLocalFrameImpl is attached to
// the DOM.

#include "web/WebLocalFrameImpl.h"

#include "bindings/core/v8/ExceptionState.h"
#include "bindings/core/v8/ExceptionStatePlaceholder.h"
#include "bindings/core/v8/ScriptController.h"
#include "bindings/core/v8/ScriptSourceCode.h"
#include "core/HTMLNames.h"
#include "core/dom/Document.h"
#include "core/dom/IconURL.h"
#include "core/dom/Node.h"
#include "core/dom/NodeTraversal.h"
#include "core/dom/SuspendableTask.h"
#include "core/dom/shadow/ShadowRoot.h"
#include "core/editing/EditingUtilities.h"
#include "core/editing/Editor.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/InputMethodController.h"
#include "core/editing/PlainTextRange.h"
#include "core/editing/TextAffinity.h"
#include "core/editing/iterators/TextIterator.h"
#include "core/editing/serializers/Serialization.h"
#include "core/editing/spellcheck/SpellChecker.h"
#include "core/fetch/ResourceFetcher.h"
#include "core/fetch/SubstituteData.h"
#include "core/frame/Console.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/FrameHost.h"
#include "core/frame/FrameView.h"
#include "core/frame/Settings.h"
#include "core/frame/UseCounter.h"
#include "core/html/HTMLAnchorElement.h"
#include "core/html/HTMLCollection.h"
#include "core/html/HTMLFormElement.h"
#include "core/html/HTMLFrameElementBase.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/html/HTMLHeadElement.h"
#include "core/html/HTMLImageElement.h"
#include "core/html/HTMLInputElement.h"
#include "core/html/HTMLLinkElement.h"
#include "core/input/EventHandler.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/layout/HitTestResult.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutObject.h"
#include "core/layout/LayoutPart.h"
#include "core/layout/LayoutTreeAsText.h"
#include "core/layout/LayoutView.h"
#include "core/style/StyleInheritedData.h"
#include "core/loader/DocumentLoader.h"
#include "core/loader/FrameLoadRequest.h"
#include "core/loader/FrameLoader.h"
#include "core/loader/HistoryItem.h"
#include "core/loader/MixedContentChecker.h"
#include "core/loader/NavigationScheduler.h"
#include "core/page/FocusController.h"
#include "core/page/FrameTree.h"
#include "core/page/Page.h"
#include "core/paint/PaintLayer.h"
#include "core/paint/ScopeRecorder.h"
#include "core/paint/TransformRecorder.h"
#include "core/timing/DOMWindowPerformance.h"
#include "core/timing/Performance.h"
#include "platform/ScriptForbiddenScope.h"
#include "platform/TraceEvent.h"
#include "platform/UserGestureIndicator.h"
#include "platform/clipboard/ClipboardUtilities.h"
#include "platform/fonts/FontCache.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/GraphicsLayerClient.h"
#include "platform/graphics/paint/ClipRecorder.h"
#include "platform/graphics/paint/DrawingRecorder.h"
#include "platform/graphics/paint/SkPictureBuilder.h"
#include "platform/graphics/skia/SkiaUtils.h"
#include "platform/heap/Handle.h"
#include "platform/network/ResourceRequest.h"
#include "platform/scroll/ScrollTypes.h"
#include "platform/scroll/ScrollbarTheme.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SchemeRegistry.h"
#include "platform/weborigin/SecurityPolicy.h"
#include "public/platform/Platform.h"
#include "public/platform/WebFloatPoint.h"
#include "public/platform/WebFloatRect.h"
#include "public/platform/WebLayer.h"
#include "public/platform/WebPoint.h"
#include "public/platform/WebRect.h"
#include "public/platform/WebSecurityOrigin.h"
#include "public/platform/WebSize.h"
#include "public/platform/WebSuspendableTask.h"
#include "public/platform/WebURLError.h"
#include "public/platform/WebVector.h"
#include "public/web/WebAutofillClient.h"
#include "public/web/WebConsoleMessage.h"
#include "public/web/WebDOMEvent.h"
#include "public/web/WebDocument.h"
#include "public/web/WebFindOptions.h"
#include "public/web/WebFormElement.h"
#include "public/web/WebFrameClient.h"
#include "public/web/WebFrameOwnerProperties.h"
#include "public/web/WebHistoryItem.h"
#include "public/web/WebIconURL.h"
#include "public/web/WebInputElement.h"
#include "public/web/WebKit.h"
#include "public/web/WebNode.h"
#include "public/web/WebPrintParams.h"
#include "public/web/WebPrintPresetOptions.h"
#include "public/web/WebRange.h"
#include "public/web/WebScriptSource.h"
#include "public/web/WebSerializedScriptValue.h"
#include "public/web/WebTreeScopeType.h"
#include "web/AssociatedURLLoader.h"
#include "web/CompositionUnderlineVectorBuilder.h"
#include "web/FindInPageCoordinates.h"
#include "web/SharedWorkerRepositoryClientImpl.h"
#include "web/TextFinder.h"
#include "web/WebDataSourceImpl.h"
#include "web/WebFrameWidgetImpl.h"
#include "web/WebViewImpl.h"
#include "wtf/CurrentTime.h"
#include "wtf/HashMap.h"
#include <algorithm>

namespace blink {

static int frameCount = 0;

// Key for a StatsCounter tracking how many WebFrames are active.
static const char webFrameActiveCount[] = "WebFrameActiveCount";

static void frameContentAsPlainText(size_t maxChars, LocalFrame* frame, StringBuilder& output)
{
    Document* document = frame->document();
    if (!document)
        return;

    if (!frame->view())
        return;

    // Select the document body.
    if (document->body()) {
        const EphemeralRange range = EphemeralRange::rangeOfContents(*document->body());

        // The text iterator will walk nodes giving us text. This is similar to
        // the plainText() function in core/editing/TextIterator.h, but we implement the maximum
        // size and also copy the results directly into a wstring, avoiding the
        // string conversion.
        for (TextIterator it(range.startPosition(), range.endPosition()); !it.atEnd(); it.advance()) {
            it.text().appendTextToStringBuilder(output, 0, maxChars - output.length());
            if (output.length() >= maxChars)
                return; // Filled up the buffer.
        }
    }

    // The separator between frames when the frames are converted to plain text.
    const LChar frameSeparator[] = { '\n', '\n' };
    const size_t frameSeparatorLength = WTF_ARRAY_LENGTH(frameSeparator);

    // Recursively walk the children.
    const FrameTree& frameTree = frame->tree();
    for (Frame* curChild = frameTree.firstChild(); curChild; curChild = curChild->tree().nextSibling()) {
        if (!curChild->isLocalFrame())
            continue;
        LocalFrame* curLocalChild = toLocalFrame(curChild);
        // Ignore the text of non-visible frames.
        LayoutView* contentLayoutObject = curLocalChild->contentLayoutObject();
        LayoutPart* ownerLayoutObject = curLocalChild->ownerLayoutObject();
        if (!contentLayoutObject || !contentLayoutObject->size().width() || !contentLayoutObject->size().height()
            || (contentLayoutObject->location().x() + contentLayoutObject->size().width() <= 0) || (contentLayoutObject->location().y() + contentLayoutObject->size().height() <= 0)
            || (ownerLayoutObject && ownerLayoutObject->style() && ownerLayoutObject->style()->visibility() != VISIBLE)) {
            continue;
        }

        // Make sure the frame separator won't fill up the buffer, and give up if
        // it will. The danger is if the separator will make the buffer longer than
        // maxChars. This will cause the computation above:
        //   maxChars - output->size()
        // to be a negative number which will crash when the subframe is added.
        if (output.length() >= maxChars - frameSeparatorLength)
            return;

        output.append(frameSeparator, frameSeparatorLength);
        frameContentAsPlainText(maxChars, curLocalChild, output);
        if (output.length() >= maxChars)
            return; // Filled up the buffer.
    }
}

static WillBeHeapVector<ScriptSourceCode> createSourcesVector(const WebScriptSource* sourcesIn, unsigned numSources)
{
    WillBeHeapVector<ScriptSourceCode> sources;
    sources.append(sourcesIn, numSources);
    return sources;
}

static WebDataSource* DataSourceForDocLoader(DocumentLoader* loader)
{
    return loader ? WebDataSourceImpl::fromDocumentLoader(loader) : 0;
}

// WebSuspendableTaskWrapper --------------------------------------------------

class WebSuspendableTaskWrapper: public SuspendableTask {
public:
    static PassOwnPtr<WebSuspendableTaskWrapper> create(PassOwnPtr<WebSuspendableTask> task)
    {
        return adoptPtr(new WebSuspendableTaskWrapper(task));
    }

    void run() override
    {
        m_task->run();
    }

    void contextDestroyed() override
    {
        m_task->contextDestroyed();
    }

private:
    explicit WebSuspendableTaskWrapper(PassOwnPtr<WebSuspendableTask> task)
        : m_task(task)
    {
    }

    OwnPtr<WebSuspendableTask> m_task;
};

// WebFrame -------------------------------------------------------------------

int WebFrame::instanceCount()
{
    return frameCount;
}

WebLocalFrame* WebLocalFrame::fromFrameOwnerElement(const WebElement& element)
{
    return WebLocalFrameImpl::fromFrameOwnerElement(PassRefPtrWillBeRawPtr<Element>(element).get());
}

bool WebLocalFrameImpl::isWebLocalFrame() const
{
    return true;
}

WebLocalFrame* WebLocalFrameImpl::toWebLocalFrame()
{
    return this;
}

void WebLocalFrameImpl::close()
{
    m_client = nullptr;

#if ENABLE(OILPAN)
    m_selfKeepAlive.clear();
#else
    deref(); // Balances ref() acquired in WebFrame::create
#endif
}

WebString WebLocalFrameImpl::uniqueName() const
{
    return frame()->tree().uniqueName();
}

WebString WebLocalFrameImpl::assignedName() const
{
    return frame()->tree().name();
}

void WebLocalFrameImpl::setName(const WebString& name)
{
    frame()->tree().setName(name);
}

WebVector<WebIconURL> WebLocalFrameImpl::iconURLs(int iconTypesMask) const
{
    // The URL to the icon may be in the header. As such, only
    // ask the loader for the icon if it's finished loading.
    if (frame()->document()->loadEventFinished())
        return frame()->document()->iconURLs(iconTypesMask);
    return WebVector<WebIconURL>();
}

void WebLocalFrameImpl::setRemoteWebLayer(WebLayer* webLayer)
{
    ASSERT_NOT_REACHED();
}

void WebLocalFrameImpl::setContentSettingsClient(WebContentSettingsClient* contentSettingsClient)
{
    m_contentSettingsClient = contentSettingsClient;
}

ScrollableArea* WebLocalFrameImpl::layoutViewportScrollableArea() const
{
    if (FrameView* view = frameView())
        return view->layoutViewportScrollableArea();
    return nullptr;
}

WebSize WebLocalFrameImpl::scrollOffset() const
{
    if (ScrollableArea* scrollableArea = layoutViewportScrollableArea())
        return toIntSize(scrollableArea->scrollPosition());
    return WebSize();
}

void WebLocalFrameImpl::setScrollOffset(const WebSize& offset)
{
    if (ScrollableArea* scrollableArea = layoutViewportScrollableArea())
        scrollableArea->setScrollPosition(IntPoint(offset.width, offset.height), ProgrammaticScroll);
}

WebSize WebLocalFrameImpl::contentsSize() const
{
    if (FrameView* view = frameView())
        return view->contentsSize();
    return WebSize();
}

bool WebLocalFrameImpl::hasVisibleContent() const
{
    if (LayoutPart* layoutObject = frame()->ownerLayoutObject()) {
        if (layoutObject->style()->visibility() != VISIBLE)
            return false;
    }

    if (FrameView* view = frameView())
        return view->visibleWidth() > 0 && view->visibleHeight() > 0;
    return false;
}

WebRect WebLocalFrameImpl::visibleContentRect() const
{
    if (FrameView* view = frameView())
        return view->visibleContentRect();
    return WebRect();
}

bool WebLocalFrameImpl::hasHorizontalScrollbar() const
{
    return frame() && frame()->view() && frame()->view()->horizontalScrollbar();
}

bool WebLocalFrameImpl::hasVerticalScrollbar() const
{
    return frame() && frame()->view() && frame()->view()->verticalScrollbar();
}

WebView* WebLocalFrameImpl::view() const
{
    return viewImpl();
}

void WebLocalFrameImpl::setOpener(WebFrame* opener)
{
    WebFrame::setOpener(opener);

    // TODO(alexmos,dcheng): This should ASSERT(m_frame) once we no longer have
    // provisional local frames.
    if (m_frame && m_frame->document())
        m_frame->document()->initSecurityContext();
}

WebDocument WebLocalFrameImpl::document() const
{
    if (!frame() || !frame()->document())
        return WebDocument();
    return WebDocument(frame()->document());
}

bool WebLocalFrameImpl::dispatchBeforeUnloadEvent()
{
    if (!frame())
        return true;
    return frame()->loader().shouldClose();
}

void WebLocalFrameImpl::dispatchUnloadEvent()
{
    if (!frame())
        return;
    SubframeLoadingDisabler disabler(frame()->document());
    frame()->loader().dispatchUnloadEvent();
}

void WebLocalFrameImpl::executeScript(const WebScriptSource& source)
{
    assert(false); // Not reached!
}

void WebLocalFrameImpl::executeScriptInIsolatedWorld(int worldID, const WebScriptSource* sourcesIn, unsigned numSources, int extensionGroup)
{
    assert(false); // Not reached!
}

void WebLocalFrameImpl::setIsolatedWorldSecurityOrigin(int worldID, const WebSecurityOrigin& securityOrigin)
{
    assert(false); // Not reached!
}

void WebLocalFrameImpl::setIsolatedWorldContentSecurityPolicy(int worldID, const WebString& policy)
{
    assert(false); // Not reached!
}

void WebLocalFrameImpl::setIsolatedWorldHumanReadableName(int worldID, const WebString& humanReadableName)
{
    assert(false); // Not reached!
}

void WebLocalFrameImpl::addMessageToConsole(const WebConsoleMessage& message)
{
    ASSERT(frame());

    MessageLevel webCoreMessageLevel;
    switch (message.level) {
    case WebConsoleMessage::LevelDebug:
        webCoreMessageLevel = DebugMessageLevel;
        break;
    case WebConsoleMessage::LevelLog:
        webCoreMessageLevel = LogMessageLevel;
        break;
    case WebConsoleMessage::LevelWarning:
        webCoreMessageLevel = WarningMessageLevel;
        break;
    case WebConsoleMessage::LevelError:
        webCoreMessageLevel = ErrorMessageLevel;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    frame()->document()->addConsoleMessage(ConsoleMessage::create(OtherMessageSource, webCoreMessageLevel, message.text, message.url, message.lineNumber, message.columnNumber));
}

void WebLocalFrameImpl::collectGarbage()
{
    assert(false); // Not reached!
}

bool WebLocalFrameImpl::checkIfRunInsecureContent(const WebURL& url) const
{
    ASSERT(frame());

    // This is only called (eventually, through proxies and delegates and IPC) from
    // PluginURLFetcher::OnReceivedRedirect for redirects of NPAPI resources.
    //
    // FIXME: Remove this method entirely once we smother NPAPI.
    return !MixedContentChecker::shouldBlockFetch(frame(), WebURLRequest::RequestContextObject, WebURLRequest::FrameTypeNested, url);
}

void WebLocalFrameImpl::requestExecuteScriptAndReturnValue(const WebScriptSource& source, bool userGesture, WebScriptExecutionCallback* callback)
{
    assert(false); // Not reached!
}

void WebLocalFrameImpl::requestExecuteScriptInIsolatedWorld(int worldID, const WebScriptSource* sourcesIn, unsigned numSources, int extensionGroup, bool userGesture, WebScriptExecutionCallback* callback)
{
    assert(false); // Not reached!
}

bool WebFrame::scriptCanAccess(WebFrame* target)
{
    assert(false); // Not reached!
    return true;
}

void WebLocalFrameImpl::reload(bool ignoreCache)
{
    // TODO(clamy): Remove this function once RenderFrame calls load for all
    // requests.
    reloadWithOverrideURL(KURL(), ignoreCache);
}

void WebLocalFrameImpl::reloadWithOverrideURL(const WebURL& overrideUrl, bool ignoreCache)
{
    // TODO(clamy): Remove this function once RenderFrame calls load for all
    // requests.
    ASSERT(frame());
    WebFrameLoadType loadType = ignoreCache ?
        WebFrameLoadType::ReloadFromOrigin : WebFrameLoadType::Reload;
    WebURLRequest request = requestForReload(loadType, overrideUrl);
    if (request.isNull())
        return;
    load(request, loadType, WebHistoryItem(), WebHistoryDifferentDocumentLoad);
}

void WebLocalFrameImpl::reloadImage(const WebNode& webNode)
{
    const Node* node = webNode.constUnwrap<Node>();
    if (isHTMLImageElement(*node)) {
        const HTMLImageElement& imageElement = toHTMLImageElement(*node);
        imageElement.forceReload();
    }
}

void WebLocalFrameImpl::loadRequest(const WebURLRequest& request)
{
    // TODO(clamy): Remove this function once RenderFrame calls load for all
    // requests.
    load(request, WebFrameLoadType::Standard, WebHistoryItem(), WebHistoryDifferentDocumentLoad);
}

void WebLocalFrameImpl::loadData(const WebData& data, const WebString& mimeType, const WebString& textEncoding, const WebURL& baseURL, const WebURL& unreachableURL, bool replace)
{
    ASSERT(frame());

    // If we are loading substitute data to replace an existing load, then
    // inherit all of the properties of that original request. This way,
    // reload will re-attempt the original request. It is essential that
    // we only do this when there is an unreachableURL since a non-empty
    // unreachableURL informs FrameLoader::reload to load unreachableURL
    // instead of the currently loaded URL.
    ResourceRequest request;
    if (replace && !unreachableURL.isEmpty() && frame()->loader().provisionalDocumentLoader())
        request = frame()->loader().provisionalDocumentLoader()->originalRequest();
    request.setURL(baseURL);
    request.setCheckForBrowserSideNavigation(false);

    FrameLoadRequest frameRequest(0, request, SubstituteData(data, mimeType, textEncoding, unreachableURL));
    ASSERT(frameRequest.substituteData().isValid());
    frameRequest.setReplacesCurrentItem(replace);
    frame()->loader().load(frameRequest);
}

void WebLocalFrameImpl::loadHTMLString(const WebData& data, const WebURL& baseURL, const WebURL& unreachableURL, bool replace)
{
    ASSERT(frame());
    loadData(data, WebString::fromUTF8("text/html"), WebString::fromUTF8("UTF-8"), baseURL, unreachableURL, replace);
}

void WebLocalFrameImpl::stopLoading()
{
    if (!frame())
        return;
    // FIXME: Figure out what we should really do here. It seems like a bug
    // that FrameLoader::stopLoading doesn't call stopAllLoaders.
    frame()->loader().stopAllLoaders();
}

WebDataSource* WebLocalFrameImpl::provisionalDataSource() const
{
    ASSERT(frame());
    return DataSourceForDocLoader(frame()->loader().provisionalDocumentLoader());
}

WebDataSource* WebLocalFrameImpl::dataSource() const
{
    ASSERT(frame());
    return DataSourceForDocLoader(frame()->loader().documentLoader());
}

void WebLocalFrameImpl::enableViewSourceMode(bool enable)
{
    if (frame())
        frame()->setInViewSourceMode(enable);
}

bool WebLocalFrameImpl::isViewSourceModeEnabled() const
{
    if (!frame())
        return false;
    return frame()->inViewSourceMode();
}

void WebLocalFrameImpl::setReferrerForRequest(WebURLRequest& request, const WebURL& referrerURL)
{
    String referrer = referrerURL.isEmpty() ? frame()->document()->outgoingReferrer() : String(referrerURL.string());
    request.toMutableResourceRequest().setHTTPReferrer(SecurityPolicy::generateReferrer(frame()->document()->referrerPolicy(), request.url(), referrer));
}

void WebLocalFrameImpl::dispatchWillSendRequest(WebURLRequest& request)
{
    ResourceResponse response;
    frame()->loader().client()->dispatchWillSendRequest(0, 0, request.toMutableResourceRequest(), response);
}

WebURLLoader* WebLocalFrameImpl::createAssociatedURLLoader(const WebURLLoaderOptions& options)
{
    return new AssociatedURLLoader(this, options);
}

unsigned WebLocalFrameImpl::unloadListenerCount() const
{
    return frame()->localDOMWindow()->pendingUnloadEventListeners();
}

void WebLocalFrameImpl::replaceSelection(const WebString& text)
{
    bool selectReplacement = frame()->editor().behavior().shouldSelectReplacement();
    bool smartReplace = true;
    frame()->editor().replaceSelectionWithText(text, selectReplacement, smartReplace);
}

void WebLocalFrameImpl::insertText(const WebString& text)
{
    if (frame()->inputMethodController().hasComposition())
        frame()->inputMethodController().confirmComposition(text);
    else
        frame()->editor().insertText(text, 0);
}

void WebLocalFrameImpl::setMarkedText(const WebString& text, unsigned location, unsigned length)
{
    Vector<CompositionUnderline> decorations;
    frame()->inputMethodController().setComposition(text, decorations, location, length);
}

void WebLocalFrameImpl::unmarkText()
{
    frame()->inputMethodController().cancelComposition();
}

bool WebLocalFrameImpl::hasMarkedText() const
{
    return frame()->inputMethodController().hasComposition();
}

WebRange WebLocalFrameImpl::markedRange() const
{
    return frame()->inputMethodController().compositionRange();
}

bool WebLocalFrameImpl::firstRectForCharacterRange(unsigned location, unsigned length, WebRect& rectInViewport) const
{
    if ((location + length < location) && (location + length))
        length = 0;

    Element* editable = frame()->selection().rootEditableElementOrDocumentElement();
    if (!editable)
        return false;
    const EphemeralRange range = PlainTextRange(location, location + length).createRange(*editable);
    if (range.isNull())
        return false;
    IntRect intRect = frame()->editor().firstRectForRange(range);
    rectInViewport = WebRect(intRect);
    rectInViewport = frame()->view()->contentsToViewport(rectInViewport);
    return true;
}

size_t WebLocalFrameImpl::characterIndexForPoint(const WebPoint& pointInViewport) const
{
    if (!frame())
        return kNotFound;

    IntPoint point = frame()->view()->viewportToContents(pointInViewport);
    HitTestResult result = frame()->eventHandler().hitTestResultAtPoint(point, HitTestRequest::ReadOnly | HitTestRequest::Active);
    const EphemeralRange range = frame()->rangeForPoint(result.roundedPointInInnerNodeFrame());
    if (range.isNull())
        return kNotFound;
    Element* editable = frame()->selection().rootEditableElementOrDocumentElement();
    ASSERT(editable);
    return PlainTextRange::create(*editable, range).start();
}

bool WebLocalFrameImpl::executeCommand(const WebString& name, const WebNode& node)
{
    ASSERT(frame());

    if (name.length() <= 2)
        return false;

    // Since we don't have NSControl, we will convert the format of command
    // string and call the function on Editor directly.
    String command = name;

    // Make sure the first letter is upper case.
    command.replace(0, 1, command.substring(0, 1).upper());

    // Remove the trailing ':' if existing.
    if (command[command.length() - 1] == UChar(':'))
        command = command.substring(0, command.length() - 1);

    return frame()->editor().executeCommand(command);
}

bool WebLocalFrameImpl::executeCommand(const WebString& name, const WebString& value, const WebNode& node)
{
    ASSERT(frame());

    return frame()->editor().executeCommand(name, value);
}

bool WebLocalFrameImpl::isCommandEnabled(const WebString& name) const
{
    ASSERT(frame());
    return frame()->editor().command(name).isEnabled();
}

void WebLocalFrameImpl::enableContinuousSpellChecking(bool enable)
{
    if (enable == isContinuousSpellCheckingEnabled())
        return;
    frame()->spellChecker().toggleContinuousSpellChecking();
}

bool WebLocalFrameImpl::isContinuousSpellCheckingEnabled() const
{
    return frame()->spellChecker().isContinuousSpellCheckingEnabled();
}

void WebLocalFrameImpl::requestTextChecking(const WebElement& webElement)
{
    if (webElement.isNull())
        return;
    frame()->spellChecker().requestTextChecking(*webElement.constUnwrap<Element>());
}

void WebLocalFrameImpl::replaceMisspelledRange(const WebString& text)
{
    // If this caret selection has two or more markers, this function replace the range covered by the first marker with the specified word as Microsoft Word does.
    frame()->spellChecker().replaceMisspelledRange(text);
}

void WebLocalFrameImpl::removeSpellingMarkers()
{
    frame()->spellChecker().removeSpellingMarkers();
}

bool WebLocalFrameImpl::hasSelection() const
{
    // frame()->selection()->isNone() never returns true.
    return frame()->selection().start() != frame()->selection().end();
}

WebRange WebLocalFrameImpl::selectionRange() const
{
    return createRange(frame()->selection().selection().toNormalizedEphemeralRange());
}

WebString WebLocalFrameImpl::selectionAsText() const
{
    String text = frame()->selection().selectedText(TextIteratorEmitsObjectReplacementCharacter);
#if OS(WIN)
    replaceNewlinesWithWindowsStyleNewlines(text);
#endif
    replaceNBSPWithSpace(text);
    return text;
}

WebString WebLocalFrameImpl::selectionAsMarkup() const
{
    return frame()->selection().selectedHTMLForClipboard();
}

void WebLocalFrameImpl::selectWordAroundPosition(LocalFrame* frame, VisiblePosition position)
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::selectWordAroundPosition");
    frame->selection().selectWordAroundPosition(position);
}

bool WebLocalFrameImpl::selectWordAroundCaret()
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::selectWordAroundCaret");
    FrameSelection& selection = frame()->selection();
    if (selection.isNone() || selection.isRange())
        return false;
    return frame()->selection().selectWordAroundPosition(selection.selection().visibleStart());
}

void WebLocalFrameImpl::selectRange(const WebPoint& baseInViewport, const WebPoint& extentInViewport)
{
    moveRangeSelection(baseInViewport, extentInViewport);
}

void WebLocalFrameImpl::selectRange(const WebRange& webRange)
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::selectRange");
    if (RefPtrWillBeRawPtr<Range> range = static_cast<PassRefPtrWillBeRawPtr<Range>>(webRange))
        frame()->selection().setSelectedRange(range.get(), VP_DEFAULT_AFFINITY, SelectionDirectionalMode::NonDirectional, NotUserTriggered);
}

void WebLocalFrameImpl::moveRangeSelectionExtent(const WebPoint& point)
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::moveRangeSelectionExtent");
    frame()->selection().moveRangeSelectionExtent(frame()->view()->viewportToContents(point));
}

void WebLocalFrameImpl::moveRangeSelection(const WebPoint& baseInViewport, const WebPoint& extentInViewport, WebFrame::TextGranularity granularity)
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::moveRangeSelection");
    blink::TextGranularity blinkGranularity = blink::CharacterGranularity;
    if (granularity == WebFrame::WordGranularity)
        blinkGranularity = blink::WordGranularity;
    frame()->selection().moveRangeSelection(
        visiblePositionForViewportPoint(baseInViewport),
        visiblePositionForViewportPoint(extentInViewport),
        blinkGranularity);
}

void WebLocalFrameImpl::moveCaretSelection(const WebPoint& pointInViewport)
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::moveCaretSelection");
    Element* editable = frame()->selection().rootEditableElement();
    if (!editable)
        return;

    VisiblePosition position = visiblePositionForViewportPoint(pointInViewport);
    frame()->selection().moveTo(position, UserTriggered);
}

bool WebLocalFrameImpl::setEditableSelectionOffsets(int start, int end)
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::setEditableSelectionOffsets");
    return frame()->inputMethodController().setEditableSelectionOffsets(PlainTextRange(start, end));
}

bool WebLocalFrameImpl::setCompositionFromExistingText(int compositionStart, int compositionEnd, const WebVector<WebCompositionUnderline>& underlines)
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::setCompositionFromExistingText");
    if (!frame()->editor().canEdit())
        return false;

    InputMethodController& inputMethodController = frame()->inputMethodController();
    inputMethodController.cancelComposition();

    if (compositionStart == compositionEnd)
        return true;

    inputMethodController.setCompositionFromExistingText(CompositionUnderlineVectorBuilder(underlines), compositionStart, compositionEnd);

    return true;
}

void WebLocalFrameImpl::extendSelectionAndDelete(int before, int after)
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::extendSelectionAndDelete");
    frame()->inputMethodController().extendSelectionAndDelete(before, after);
}

void WebLocalFrameImpl::setCaretVisible(bool visible)
{
    frame()->selection().setCaretVisible(visible);
}

VisiblePosition WebLocalFrameImpl::visiblePositionForViewportPoint(const WebPoint& pointInViewport)
{
    return visiblePositionForContentsPoint(frame()->view()->viewportToContents(pointInViewport), frame());
}

bool WebLocalFrameImpl::hasCustomPageSizeStyle(int pageIndex)
{
#ifdef BLINKIT_CRAWLER_ONLY
    assert(false); // BKTODO: Not reached!
    return false;
#else
    return frame()->document()->styleForPage(pageIndex)->pageSizeType() != PAGE_SIZE_AUTO;
#endif
}

bool WebLocalFrameImpl::isPageBoxVisible(int pageIndex)
{
    return frame()->document()->isPageBoxVisible(pageIndex);
}

void WebLocalFrameImpl::pageSizeAndMarginsInPixels(int pageIndex, WebSize& pageSize, int& marginTop, int& marginRight, int& marginBottom, int& marginLeft)
{
    IntSize size = pageSize;
    frame()->document()->pageSizeAndMarginsInPixels(pageIndex, size, marginTop, marginRight, marginBottom, marginLeft);
    pageSize = size;
}

WebString WebLocalFrameImpl::pageProperty(const WebString& propertyName, int pageIndex)
{
    assert(false); // Not reached!
    return WebString();
}

bool WebLocalFrameImpl::find(int identifier, const WebString& searchText, const WebFindOptions& options, bool wrapWithinFrame, WebRect* selectionRect)
{
    return ensureTextFinder().find(identifier, searchText, options, wrapWithinFrame, selectionRect);
}

void WebLocalFrameImpl::stopFinding(bool clearSelection)
{
    if (m_textFinder) {
        if (!clearSelection)
            setFindEndstateFocusAndSelection();
        m_textFinder->stopFindingAndClearSelection();
    }
}

void WebLocalFrameImpl::scopeStringMatches(int identifier, const WebString& searchText, const WebFindOptions& options, bool reset)
{
    ensureTextFinder().scopeStringMatches(identifier, searchText, options, reset);
}

void WebLocalFrameImpl::cancelPendingScopingEffort()
{
    if (m_textFinder)
        m_textFinder->cancelPendingScopingEffort();
}

void WebLocalFrameImpl::increaseMatchCount(int count, int identifier)
{
    // This function should only be called on the mainframe.
    ASSERT(!parent());
    ensureTextFinder().increaseMatchCount(identifier, count);
}

void WebLocalFrameImpl::resetMatchCount()
{
    ensureTextFinder().resetMatchCount();
}

void WebLocalFrameImpl::dispatchMessageEventWithOriginCheck(const WebSecurityOrigin& intendedTargetOrigin, const WebDOMEvent& event)
{
    assert(false); // Not reached!
}

int WebLocalFrameImpl::findMatchMarkersVersion() const
{
    ASSERT(!parent());

    if (m_textFinder)
        return m_textFinder->findMatchMarkersVersion();
    return 0;
}

int WebLocalFrameImpl::selectNearestFindMatch(const WebFloatPoint& point, WebRect* selectionRect)
{
    ASSERT(!parent());
    return ensureTextFinder().selectNearestFindMatch(point, selectionRect);
}

WebFloatRect WebLocalFrameImpl::activeFindMatchRect()
{
    ASSERT(!parent());

    if (m_textFinder)
        return m_textFinder->activeFindMatchRect();
    return WebFloatRect();
}

void WebLocalFrameImpl::findMatchRects(WebVector<WebFloatRect>& outputRects)
{
    ASSERT(!parent());
    ensureTextFinder().findMatchRects(outputRects);
}

void WebLocalFrameImpl::setTickmarks(const WebVector<WebRect>& tickmarks)
{
    if (frameView()) {
        Vector<IntRect> tickmarksConverted(tickmarks.size());
        for (size_t i = 0; i < tickmarks.size(); ++i)
            tickmarksConverted[i] = tickmarks[i];
        frameView()->setTickmarks(tickmarksConverted);
    }
}

WebString WebLocalFrameImpl::contentAsText(size_t maxChars) const
{
    if (!frame())
        return WebString();
    StringBuilder text;
    frameContentAsPlainText(maxChars, frame(), text);
    return text.toString();
}

WebString WebLocalFrameImpl::contentAsMarkup() const
{
    if (!frame())
        return WebString();
    return createMarkup(frame()->document());
}

WebString WebLocalFrameImpl::layoutTreeAsText(LayoutAsTextControls toShow) const
{
    LayoutAsTextBehavior behavior = LayoutAsTextShowAllLayers;

    if (toShow & LayoutAsTextWithLineTrees)
        behavior |= LayoutAsTextShowLineTrees;

    if (toShow & LayoutAsTextDebug)
        behavior |= LayoutAsTextShowCompositedLayers | LayoutAsTextShowAddresses | LayoutAsTextShowIDAndClass | LayoutAsTextShowLayerNesting;

    if (toShow & LayoutAsTextPrinting)
        behavior |= LayoutAsTextPrintingMode;

    return externalRepresentation(frame(), behavior);
}

WebString WebLocalFrameImpl::markerTextForListItem(const WebElement& webElement) const
{
    return blink::markerTextForListItem(const_cast<Element*>(webElement.constUnwrap<Element>()));
}

WebRect WebLocalFrameImpl::selectionBoundsRect() const
{
    return hasSelection() ? WebRect(IntRect(frame()->selection().bounds())) : WebRect();
}

bool WebLocalFrameImpl::selectionStartHasSpellingMarkerFor(int from, int length) const
{
    if (!frame())
        return false;
    return frame()->spellChecker().selectionStartHasSpellingMarkerFor(from, length);
}

WebString WebLocalFrameImpl::layerTreeAsText(bool showDebugInfo) const
{
    if (!frame())
        return WebString();

    return WebString(frame()->layerTreeAsText(showDebugInfo ? LayerTreeIncludesDebugInfo : LayerTreeNormal));
}

// WebLocalFrameImpl public ---------------------------------------------------------

WebLocalFrame* WebLocalFrame::create(WebTreeScopeType scope, WebFrameClient* client)
{
    return WebLocalFrameImpl::create(scope, client);
}

WebLocalFrame* WebLocalFrame::createProvisional(WebFrameClient* client, WebRemoteFrame* oldWebFrame, WebSandboxFlags flags, const WebFrameOwnerProperties& frameOwnerProperties)
{
    return WebLocalFrameImpl::createProvisional(client, oldWebFrame, flags, frameOwnerProperties);
}

WebLocalFrameImpl* WebLocalFrameImpl::create(WebTreeScopeType scope, WebFrameClient* client)
{
    WebLocalFrameImpl* frame = new WebLocalFrameImpl(scope, client);
#if ENABLE(OILPAN)
    return frame;
#else
    return adoptRef(frame).leakRef();
#endif
}

WebLocalFrameImpl* WebLocalFrameImpl::createProvisional(WebFrameClient* client, WebRemoteFrame* oldWebFrame, WebSandboxFlags flags, const WebFrameOwnerProperties& frameOwnerProperties)
{
    assert(false); // Not reached!
    return nullptr;
}


WebLocalFrameImpl::WebLocalFrameImpl(WebTreeScopeType scope, WebFrameClient* client)
    : WebLocalFrame(scope)
    , m_frameLoaderClientImpl(FrameLoaderClientImpl::create(this))
    , m_frameWidget(0)
    , m_client(client)
    , m_autofillClient(0)
    , m_contentSettingsClient(0)
    , m_inputEventsScaleFactorForEmulation(1)
#if ENABLE(OILPAN)
    , m_selfKeepAlive(this)
#endif
{
    Platform::current()->incrementStatsCounter(webFrameActiveCount);
    frameCount++;
}

WebLocalFrameImpl::~WebLocalFrameImpl()
{
    // The widget for the frame, if any, must have already been closed.
    ASSERT(!m_frameWidget);
    Platform::current()->decrementStatsCounter(webFrameActiveCount);
    frameCount--;

#if !ENABLE(OILPAN)
    cancelPendingScopingEffort();
#endif
}

#if ENABLE(OILPAN)
DEFINE_TRACE(WebLocalFrameImpl)
{
    visitor->trace(m_frameLoaderClientImpl);
    visitor->trace(m_frame);
    visitor->trace(m_devToolsAgent);
    visitor->trace(m_textFinder);
    visitor->trace(m_geolocationClientProxy);
    visitor->template registerWeakMembers<WebFrame, &WebFrame::clearWeakFrames>(this);
    WebFrame::traceFrames(visitor, this);
    WebFrameImplBase::trace(visitor);
}
#endif

void WebLocalFrameImpl::setCoreFrame(PassRefPtrWillBeRawPtr<LocalFrame> frame)
{
    m_frame = frame;
}

void WebLocalFrameImpl::initializeCoreFrame(FrameHost* host, FrameOwner* owner, const AtomicString& name, const AtomicString& fallbackName)
{
    setCoreFrame(LocalFrame::create(m_frameLoaderClientImpl.get(), host, owner));
    frame()->tree().setName(name, fallbackName);
    // We must call init() after m_frame is assigned because it is referenced
    // during init(). Note that this may dispatch JS events; the frame may be
    // detached after init() returns.
    frame()->init();
}

PassRefPtrWillBeRawPtr<LocalFrame> WebLocalFrameImpl::createChildFrame(const FrameLoadRequest& request,
    const AtomicString& name, HTMLFrameOwnerElement* ownerElement)
{
    ASSERT(m_client);
    TRACE_EVENT0("blink", "WebLocalFrameImpl::createChildframe");
    WebTreeScopeType scope = frame()->document() == ownerElement->treeScope()
        ? WebTreeScopeType::Document
        : WebTreeScopeType::Shadow;
    WebFrameOwnerProperties ownerProperties(ownerElement->scrollingMode(), ownerElement->marginWidth(), ownerElement->marginHeight());
    RefPtrWillBeRawPtr<WebLocalFrameImpl> webframeChild = toWebLocalFrameImpl(m_client->createChildFrame(this, scope, name, ownerProperties));
    if (!webframeChild)
        return nullptr;

    // FIXME: Using subResourceAttributeName as fallback is not a perfect
    // solution. subResourceAttributeName returns just one attribute name. The
    // element might not have the attribute, and there might be other attributes
    // which can identify the element.
    webframeChild->initializeCoreFrame(frame()->host(), ownerElement, name, ownerElement->getAttribute(ownerElement->subResourceAttributeName()));
    // Initializing the core frame may cause the new child to be detached, since
    // it may dispatch a load event in the parent.
    if (!webframeChild->parent())
        return nullptr;

    FrameLoadRequest newRequest = request;
    webframeChild->frame()->loader().load(newRequest, FrameLoadTypeStandard, nullptr);

    // Note a synchronous navigation (about:blank) would have already processed
    // onload, so it is possible for the child frame to have already been
    // detached by script in the page.
    if (!webframeChild->parent())
        return nullptr;
    return webframeChild->frame();
}

void WebLocalFrameImpl::didChangeContentsSize(const IntSize& size)
{
    // This is only possible on the main frame.
    if (m_textFinder && m_textFinder->totalMatchCount() > 0) {
        ASSERT(!parent());
        m_textFinder->increaseMarkerVersion();
    }
}

void WebLocalFrameImpl::createFrameView()
{
    TRACE_EVENT0("blink", "WebLocalFrameImpl::createFrameView");

    ASSERT(frame()); // If frame() doesn't exist, we probably didn't init properly.

    WebViewImpl* webView = viewImpl();

    bool isMainFrame = !parent();
    IntSize initialSize = (isMainFrame || !frameWidget()) ? webView->mainFrameSize() : (IntSize)frameWidget()->size();

    frame()->createView(initialSize, webView->baseBackgroundColor(), webView->isTransparent());
    if (webView->shouldAutoResize() && frame()->isLocalRoot())
        frame()->view()->enableAutoSizeMode(webView->minAutoSize(), webView->maxAutoSize());

    frame()->view()->setInputEventsTransformForEmulation(m_inputEventsOffsetForEmulation, m_inputEventsScaleFactorForEmulation);
    frame()->view()->setDisplayMode(webView->displayMode());
}

WebLocalFrameImpl* WebLocalFrameImpl::fromFrame(LocalFrame* frame)
{
    if (!frame)
        return nullptr;
    return fromFrame(*frame);
}

WebLocalFrameImpl* WebLocalFrameImpl::fromFrame(LocalFrame& frame)
{
    FrameLoaderClient* client = frame.loader().client();
    if (!client || !client->isFrameLoaderClientImpl())
        return nullptr;
    return toFrameLoaderClientImpl(client)->webFrame();
}

WebLocalFrameImpl* WebLocalFrameImpl::fromFrameOwnerElement(Element* element)
{
    if (!element->isFrameOwnerElement())
        return nullptr;
    return fromFrame(toLocalFrame(toHTMLFrameOwnerElement(element)->contentFrame()));
}

WebViewImpl* WebLocalFrameImpl::viewImpl() const
{
    if (!frame())
        return nullptr;
    return WebViewImpl::fromPage(frame()->page());
}

WebDataSourceImpl* WebLocalFrameImpl::dataSourceImpl() const
{
    return static_cast<WebDataSourceImpl*>(dataSource());
}

WebDataSourceImpl* WebLocalFrameImpl::provisionalDataSourceImpl() const
{
    return static_cast<WebDataSourceImpl*>(provisionalDataSource());
}

void WebLocalFrameImpl::setFindEndstateFocusAndSelection()
{
    WebLocalFrameImpl* mainFrameImpl = viewImpl()->mainFrameImpl();

    if (this != mainFrameImpl->activeMatchFrame())
        return;

    if (Range* activeMatch = m_textFinder->activeMatch()) {
        // If the user has set the selection since the match was found, we
        // don't focus anything.
        VisibleSelection selection(frame()->selection().selection());
        if (!selection.isNone())
            return;

        // Need to clean out style and layout state before querying Element::isFocusable().
        frame()->document()->updateLayoutIgnorePendingStylesheets();

        // Try to find the first focusable node up the chain, which will, for
        // example, focus links if we have found text within the link.
        Node* node = activeMatch->firstNode();
        if (node && node->isInShadowTree()) {
            if (Node* host = node->shadowHost()) {
                if (isHTMLInputElement(*host) || isHTMLTextAreaElement(*host))
                    node = host;
            }
        }
        for (; node; node = node->parentNode()) {
            if (!node->isElementNode())
                continue;
            Element* element = toElement(node);
            if (element->isFocusable()) {
                // Found a focusable parent node. Set the active match as the
                // selection and focus to the focusable node.
                frame()->selection().setSelection(VisibleSelection(EphemeralRange(activeMatch)));
                frame()->document()->setFocusedElement(element, FocusParams(SelectionBehaviorOnFocus::None, WebFocusTypeNone, nullptr));
                return;
            }
        }

        // Iterate over all the nodes in the range until we find a focusable node.
        // This, for example, sets focus to the first link if you search for
        // text and text that is within one or more links.
        node = activeMatch->firstNode();
        for (; node && node != activeMatch->pastLastNode(); node = NodeTraversal::next(*node)) {
            if (!node->isElementNode())
                continue;
            Element* element = toElement(node);
            if (element->isFocusable()) {
                frame()->document()->setFocusedElement(element, FocusParams(SelectionBehaviorOnFocus::None, WebFocusTypeNone, nullptr));
                return;
            }
        }

        // No node related to the active match was focusable, so set the
        // active match as the selection (so that when you end the Find session,
        // you'll have the last thing you found highlighted) and make sure that
        // we have nothing focused (otherwise you might have text selected but
        // a link focused, which is weird).
        frame()->selection().setSelection(VisibleSelection(EphemeralRange(activeMatch)));
        frame()->document()->clearFocusedElement();

        // Finally clear the active match, for two reasons:
        // We just finished the find 'session' and we don't want future (potentially
        // unrelated) find 'sessions' operations to start at the same place.
        // The WebLocalFrameImpl could get reused and the activeMatch could end up pointing
        // to a document that is no longer valid. Keeping an invalid reference around
        // is just asking for trouble.
        m_textFinder->resetActiveMatch();
    }
}

void WebLocalFrameImpl::didFail(const ResourceError& error, bool wasProvisional, HistoryCommitType commitType)
{
    if (!client())
        return;
    WebURLError webError = error;
    WebHistoryCommitType webCommitType = static_cast<WebHistoryCommitType>(commitType);

    if (wasProvisional)
        client()->didFailProvisionalLoad(this, webError, webCommitType);
    else
        client()->didFailLoad(this, webError, webCommitType);
}

void WebLocalFrameImpl::didFinish()
{
    if (!client())
        return;

    client()->didFinishLoad(this);
}

void WebLocalFrameImpl::setCanHaveScrollbars(bool canHaveScrollbars)
{
    frame()->view()->setCanHaveScrollbars(canHaveScrollbars);
}

void WebLocalFrameImpl::setInputEventsTransformForEmulation(const IntSize& offset, float contentScaleFactor)
{
    m_inputEventsOffsetForEmulation = offset;
    m_inputEventsScaleFactorForEmulation = contentScaleFactor;
    if (frame()->view())
        frame()->view()->setInputEventsTransformForEmulation(m_inputEventsOffsetForEmulation, m_inputEventsScaleFactorForEmulation);
}

static void ensureFrameLoaderHasCommitted(FrameLoader& frameLoader)
{
    // Internally, Blink uses CommittedMultipleRealLoads to track whether the
    // next commit should create a new history item or not. Ensure we have
    // reached that state.
    if (frameLoader.stateMachine()->committedMultipleRealLoads())
        return;
    frameLoader.stateMachine()->advanceTo(FrameLoaderStateMachine::CommittedMultipleRealLoads);
}

void WebLocalFrameImpl::setAutofillClient(WebAutofillClient* autofillClient)
{
    m_autofillClient = autofillClient;
}

WebAutofillClient* WebLocalFrameImpl::autofillClient()
{
    return m_autofillClient;
}

void WebLocalFrameImpl::setFrameOwnerProperties(const WebFrameOwnerProperties& frameOwnerProperties)
{
    assert(false); // Not reached!
}

WebLocalFrameImpl* WebLocalFrameImpl::localRoot()
{
    // This can't use the LocalFrame::localFrameRoot, since it may be called
    // when the WebLocalFrame exists but the core LocalFrame does not.
    // TODO(alexmos, dcheng): Clean this up to only calculate this in one place.
    WebLocalFrameImpl* localRoot = this;
    while (!localRoot->frameWidget())
        localRoot = toWebLocalFrameImpl(localRoot->parent());
    return localRoot;
}

void WebLocalFrameImpl::sendPings(const WebNode& contextNode, const WebURL& destinationURL)
{
    ASSERT(frame());
    Element* anchor = contextNode.constUnwrap<Node>()->enclosingLinkEventParentOrSelf();
    if (isHTMLAnchorElement(anchor))
        toHTMLAnchorElement(anchor)->sendPings(destinationURL);
}

WebURLRequest WebLocalFrameImpl::requestForReload(WebFrameLoadType loadType,
    const WebURL& overrideUrl) const
{
    ASSERT(frame());
    ResourceRequest request = frame()->loader().resourceRequestForReload(
        static_cast<FrameLoadType>(loadType), overrideUrl);
    return WrappedResourceRequest(request);
}

void WebLocalFrameImpl::load(const WebURLRequest& request, WebFrameLoadType webFrameLoadType,
    const WebHistoryItem& item, WebHistoryLoadType webHistoryLoadType)
{
    ASSERT(frame());
    ASSERT(!request.isNull());
    const ResourceRequest& resourceRequest = request.toResourceRequest();

    if (resourceRequest.url().protocolIs("javascript")
        && webFrameLoadType == WebFrameLoadType::Standard) {
        assert(false); // Not reached!
        return;
    }

    FrameLoadRequest frameRequest = FrameLoadRequest(nullptr, resourceRequest);
    RefPtrWillBeRawPtr<HistoryItem> historyItem = PassRefPtrWillBeRawPtr<HistoryItem>(item);
    frame()->loader().load(
        frameRequest, static_cast<FrameLoadType>(webFrameLoadType), historyItem.get(),
        static_cast<HistoryLoadType>(webHistoryLoadType));
}

bool WebLocalFrameImpl::isLoading() const
{
    if (!frame() || !frame()->document())
        return false;
    return frame()->loader().stateMachine()->isDisplayingInitialEmptyDocument() || frame()->loader().provisionalDocumentLoader() || !frame()->document()->loadEventFinished();
}

bool WebLocalFrameImpl::isResourceLoadInProgress() const
{
    if (!frame() || !frame()->document())
        return false;
    return frame()->document()->fetcher()->requestCount();
}

bool WebLocalFrameImpl::isNavigationScheduled() const
{
    return frame() && frame()->navigationScheduler().isNavigationScheduled();
}

void WebLocalFrameImpl::setCommittedFirstRealLoad()
{
    ASSERT(frame());
    ensureFrameLoaderHasCommitted(frame()->loader());
}

void WebLocalFrameImpl::requestRunTask(WebSuspendableTask* task) const
{
    ASSERT(frame());
    ASSERT(frame()->document());
    frame()->document()->postSuspendableTask(WebSuspendableTaskWrapper::create(adoptPtr(task)));
}

void WebLocalFrameImpl::didCallAddSearchProvider()
{
    UseCounter::count(frame(), UseCounter::ExternalAddSearchProvider);
}

void WebLocalFrameImpl::didCallIsSearchProviderInstalled()
{
    UseCounter::count(frame(), UseCounter::ExternalIsSearchProviderInstalled);
}

void WebLocalFrameImpl::willBeDetached()
{
    // Currently nothing to do.
}

void WebLocalFrameImpl::willDetachParent()
{
    // Do not expect string scoping results from any frames that got detached
    // in the middle of the operation.
    if (m_textFinder && m_textFinder->scopingInProgress()) {

        // There is a possibility that the frame being detached was the only
        // pending one. We need to make sure final replies can be sent.
        m_textFinder->flushCurrentScoping();

        m_textFinder->cancelPendingScopingEffort();
    }
}

WebLocalFrameImpl* WebLocalFrameImpl::activeMatchFrame() const
{
    ASSERT(!parent());

    if (m_textFinder)
        return m_textFinder->activeMatchFrame();
    return 0;
}

Range* WebLocalFrameImpl::activeMatch() const
{
    if (m_textFinder)
        return m_textFinder->activeMatch();
    return 0;
}

TextFinder& WebLocalFrameImpl::ensureTextFinder()
{
    if (!m_textFinder)
        m_textFinder = TextFinder::create(*this);

    return *m_textFinder;
}

void WebLocalFrameImpl::setFrameWidget(WebFrameWidget* frameWidget)
{
    m_frameWidget = frameWidget;
}

WebFrameWidget* WebLocalFrameImpl::frameWidget() const
{
    return m_frameWidget;
}

} // namespace blink
