// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: WebLocalFrameImpl.h
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

#ifndef WebLocalFrameImpl_h
#define WebLocalFrameImpl_h

#include "core/editing/VisiblePosition.h"
#include "core/frame/LocalFrame.h"
#include "platform/geometry/FloatRect.h"
#include "public/platform/WebFileSystemType.h"
#include "public/web/WebLocalFrame.h"
#include "web/FrameLoaderClientImpl.h"
#include "web/WebFrameImplBase.h"
#include "wtf/Compiler.h"
#include "wtf/OwnPtr.h"
#include "wtf/RefCounted.h"
#include "wtf/text/WTFString.h"

namespace blink {

class IntSize;
class KURL;
class Range;
class WebAutofillClient;
class WebDataSourceImpl;
class WebDevToolsAgentImpl;
class WebDevToolsFrontendImpl;
class WebFrameClient;
class WebFrameWidget;
class WebNode;
class WebScriptExecutionCallback;
class WebSuspendableTask;
class WebView;
class WebViewImpl;
struct FrameLoadRequest;

template <typename T> class WebVector;

// Implementation of WebFrame, note that this is a reference counted object.
class WebLocalFrameImpl final : public WebFrameImplBase, public WebLocalFrame {
public:
    // WebFrame methods:
    bool isWebLocalFrame() const override;
    WebLocalFrame* toWebLocalFrame() override;
    void close() override;
    WebString uniqueName() const override;
    WebString assignedName() const override;
    void setName(const WebString&) override;
    WebVector<WebIconURL> iconURLs(int iconTypesMask) const override;
    void setRemoteWebLayer(WebLayer*) override;
    void setContentSettingsClient(WebContentSettingsClient*) override;
    WebSize scrollOffset() const override;
    void setScrollOffset(const WebSize&) override;
    WebSize contentsSize() const override;
    bool hasVisibleContent() const override;
    WebRect visibleContentRect() const override;
    bool hasHorizontalScrollbar() const override;
    bool hasVerticalScrollbar() const override;
    WebView* view() const override;
    void setOpener(WebFrame*) override;
    WebDocument document() const override;
    bool dispatchBeforeUnloadEvent() override;
    void dispatchUnloadEvent() override;
    void executeScript(const WebScriptSource&) override;
    void executeScriptInIsolatedWorld(
        int worldID, const WebScriptSource* sources, unsigned numSources,
        int extensionGroup) override;
    void setIsolatedWorldSecurityOrigin(int worldID, const WebSecurityOrigin&) override;
    void setIsolatedWorldContentSecurityPolicy(int worldID, const WebString&) override;
    void setIsolatedWorldHumanReadableName(int worldID, const WebString&) override;
    void addMessageToConsole(const WebConsoleMessage&) override;
    void collectGarbage() override;
    bool checkIfRunInsecureContent(const WebURL&) const override;
    void requestExecuteScriptAndReturnValue(
        const WebScriptSource&, bool userGesture, WebScriptExecutionCallback*) override;
    void requestExecuteScriptInIsolatedWorld(
        int worldID, const WebScriptSource* sourceIn, unsigned numSources,
        int extensionGroup, bool userGesture, WebScriptExecutionCallback*) override;
    void reload(bool ignoreCache) override;
    void reloadWithOverrideURL(const WebURL& overrideUrl, bool ignoreCache) override;
    void reloadImage(const WebNode&) override;
    void loadRequest(const WebURLRequest&) override;
    void loadData(
        const WebData&, const WebString& mimeType, const WebString& textEncoding,
        const WebURL& baseURL, const WebURL& unreachableURL, bool replace) override;
    void loadHTMLString(
        const WebData& html, const WebURL& baseURL, const WebURL& unreachableURL,
        bool replace) override;
    void stopLoading() override;
    WebDataSource* provisionalDataSource() const override;
    WebDataSource* dataSource() const override;
    void setReferrerForRequest(WebURLRequest&, const WebURL& referrer) override;
    void dispatchWillSendRequest(WebURLRequest&) override;
    WebURLLoader* createAssociatedURLLoader(const WebURLLoaderOptions&) override;
    unsigned unloadListenerCount() const override;
    void insertText(const WebString&) override;
    void setMarkedText(const WebString&, unsigned location, unsigned length) override;
    void unmarkText() override;
    bool hasMarkedText() const override;
    WebRange markedRange() const override;
    bool firstRectForCharacterRange(unsigned location, unsigned length, WebRect&) const override;
    size_t characterIndexForPoint(const WebPoint&) const override;
    bool executeCommand(const WebString&, const WebNode& = WebNode()) override;
    bool executeCommand(const WebString&, const WebString& value, const WebNode& = WebNode()) override;
    bool isCommandEnabled(const WebString&) const override;
    void enableContinuousSpellChecking(bool) override;
    bool isContinuousSpellCheckingEnabled() const override;
    void requestTextChecking(const WebElement&) override;
    void replaceMisspelledRange(const WebString&) override;
    void removeSpellingMarkers() override;
    bool hasSelection() const override;
    WebRange selectionRange() const override;
    WebString selectionAsText() const override;
    WebString selectionAsMarkup() const override;
    bool selectWordAroundCaret() override;
    void selectRange(const WebPoint& base, const WebPoint& extent) override;
    void selectRange(const WebRange&) override;
    void moveRangeSelectionExtent(const WebPoint&) override;
    void moveRangeSelection(const WebPoint& base, const WebPoint& extent, WebFrame::TextGranularity = CharacterGranularity) override;
    void moveCaretSelection(const WebPoint&) override;
    bool setEditableSelectionOffsets(int start, int end) override;
    bool setCompositionFromExistingText(int compositionStart, int compositionEnd, const WebVector<WebCompositionUnderline>& underlines) override;
    void extendSelectionAndDelete(int before, int after) override;
    void setCaretVisible(bool) override;
    bool hasCustomPageSizeStyle(int pageIndex) override;
    bool isPageBoxVisible(int pageIndex) override;
    void pageSizeAndMarginsInPixels(
        int pageIndex,
        WebSize& pageSize,
        int& marginTop,
        int& marginRight,
        int& marginBottom,
        int& marginLeft) override;
    WebString pageProperty(const WebString& propertyName, int pageIndex) override;
    void setTickmarks(const WebVector<WebRect>&) override;

    void dispatchMessageEventWithOriginCheck(
        const WebSecurityOrigin& intendedTargetOrigin,
        const WebDOMEvent&) override;

    WebString contentAsText(size_t maxChars) const override;
    WebString contentAsMarkup() const override;
    WebString layoutTreeAsText(LayoutAsTextControls toShow = LayoutAsTextNormal) const override;

    WebString markerTextForListItem(const WebElement&) const override;
    WebRect selectionBoundsRect() const override;

    bool selectionStartHasSpellingMarkerFor(int from, int length) const override;
    WebString layerTreeAsText(bool showDebugInfo = false) const override;

    WebFrameImplBase* toImplBase() override { return this; }

    // WebLocalFrame methods:
    void setAutofillClient(WebAutofillClient*) override;
    WebAutofillClient* autofillClient() override;
    void setFrameOwnerProperties(const WebFrameOwnerProperties&) override;
    WebLocalFrameImpl* localRoot() override;
    void sendPings(const WebNode& contextNode, const WebURL& destinationURL) override;
    WebURLRequest requestForReload(WebFrameLoadType, const WebURL&) const override;
    void load(const WebURLRequest&, WebFrameLoadType, const WebHistoryItem&,
        WebHistoryLoadType) override;
    bool isLoading() const override;
    bool isResourceLoadInProgress() const override;
    bool isNavigationScheduled() const override;
    void setCommittedFirstRealLoad() override;
    void requestRunTask(WebSuspendableTask*) const override;
    void didCallAddSearchProvider() override;
    void didCallIsSearchProviderInstalled() override;
    void replaceSelection(const WebString&) override;

    // WebFrameImplBase methods:
    void initializeCoreFrame(FrameHost*, FrameOwner*, const AtomicString& name, const AtomicString& fallbackName) override;
    LocalFrame* frame() const override { return m_frame.get(); }

    void willBeDetached();
    void willDetachParent();

    static WebLocalFrameImpl* create(WebTreeScopeType, WebFrameClient*);
    static WebLocalFrameImpl* createProvisional(WebFrameClient*, WebRemoteFrame*, WebSandboxFlags, const WebFrameOwnerProperties&);
    ~WebLocalFrameImpl() override;

    PassRefPtrWillBeRawPtr<LocalFrame> createChildFrame(const FrameLoadRequest&, const AtomicString& name, HTMLFrameOwnerElement*);

    void didChangeContentsSize(const IntSize&);

    void createFrameView();

    static WebLocalFrameImpl* fromFrame(LocalFrame*);
    static WebLocalFrameImpl* fromFrame(LocalFrame&);
    static WebLocalFrameImpl* fromFrameOwnerElement(Element*);

    WebViewImpl* viewImpl() const;

    FrameView* frameView() const { return frame() ? frame()->view() : 0; }

    // Getters for the impls corresponding to Get(Provisional)DataSource. They
    // may return 0 if there is no corresponding data source.
    WebDataSourceImpl* dataSourceImpl() const;
    WebDataSourceImpl* provisionalDataSourceImpl() const;

    void didFail(const ResourceError&, bool wasProvisional, HistoryCommitType);
    void didFinish();

    // Sets whether the WebLocalFrameImpl allows its document to be scrolled.
    // If the parameter is true, allow the document to be scrolled.
    // Otherwise, disallow scrolling.
    void setCanHaveScrollbars(bool) override;

    WebFrameClient* client() const { return m_client; }
    void setClient(WebFrameClient* client) { m_client = client; }

    WebContentSettingsClient* contentSettingsClient() { return m_contentSettingsClient; }

    void setInputEventsTransformForEmulation(const IntSize&, float);

    static void selectWordAroundPosition(LocalFrame*, VisiblePosition);

    // Returns a hit-tested VisiblePosition for the given point
    VisiblePosition visiblePositionForViewportPoint(const WebPoint&);

    void setFrameWidget(WebFrameWidget*);
    WebFrameWidget* frameWidget() const;

#if ENABLE(OILPAN)
    DECLARE_TRACE();
#endif

private:
    friend class FrameLoaderClientImpl;

    WebLocalFrameImpl(WebTreeScopeType, WebFrameClient*);

    // Sets the local core frame and registers destruction observers.
    void setCoreFrame(PassRefPtrWillBeRawPtr<LocalFrame>);

    ScrollableArea* layoutViewportScrollableArea() const;

    OwnPtrWillBeMember<FrameLoaderClientImpl> m_frameLoaderClientImpl;

    // The embedder retains a reference to the WebCore LocalFrame while it is active in the DOM. This
    // reference is released when the frame is removed from the DOM or the entire page is closed.
    // FIXME: These will need to change to WebFrame when we introduce WebFrameProxy.
    RefPtrWillBeMember<LocalFrame> m_frame;

    // This is set if the frame is the root of a local frame tree, and requires a widget for layout.
    WebFrameWidget* m_frameWidget;

    WebFrameClient* m_client;
    WebAutofillClient* m_autofillClient;
    WebContentSettingsClient* m_contentSettingsClient;

    // Stores the additional input events offset and scale when device metrics emulation is enabled.
    IntSize m_inputEventsOffsetForEmulation;
    float m_inputEventsScaleFactorForEmulation;

#if ENABLE(OILPAN)
    // Oilpan: WebLocalFrameImpl must remain alive until close() is called.
    // Accomplish that by keeping a self-referential Persistent<>. It is
    // cleared upon close().
    SelfKeepAlive<WebLocalFrameImpl> m_selfKeepAlive;
#endif
};

DEFINE_TYPE_CASTS(WebLocalFrameImpl, WebFrame, frame, frame->isWebLocalFrame(), frame.isWebLocalFrame());

} // namespace blink

#endif
