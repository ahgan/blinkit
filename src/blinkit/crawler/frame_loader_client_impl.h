// -------------------------------------------------
// BlinKit - BlinKit Library
// -------------------------------------------------
//   File Name: frame_loader_client_impl.h
// Description: FrameLoaderClientImpl Class
//      Author: Ziming Li
//     Created: 2019-04-10
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

#ifndef BLINKIT_BLINKIT_FRAME_LOADER_CLIENT_IMPL_H
#define BLINKIT_BLINKIT_FRAME_LOADER_CLIENT_IMPL_H

#pragma once

#include "core/loader/FrameLoaderClient.h"

namespace BlinKit {

class FrameLoaderClientImpl : public blink::FrameLoaderClient
{
protected:
    // blink::FrameClient
    bool inShadowTree(void) const override final;
    void willBeDetached(void) override final;
    void detached(blink::FrameDetachType) override final;
    blink::Frame* opener(void) const override final;
    void setOpener(blink::Frame *) override final;
    blink::Frame* parent(void) const override final;
    blink::Frame* top(void) const override final;
    blink::Frame* previousSibling(void) const override final;
    blink::Frame* nextSibling(void) const override final;
    blink::Frame* firstChild(void) const override final;
    blink::Frame* lastChild(void) const override final;
    unsigned backForwardLength(void) override final;
    void frameFocused(void) const override final;
    // blink::FrameLoaderClient
    bool hasWebView(void) const override final;
    void dispatchWillSendRequest(blink::DocumentLoader *, unsigned long identifier, blink::ResourceRequest &, const blink::ResourceResponse &redirectResponse) override final;
    void dispatchDidReceiveResponse(blink::DocumentLoader *, unsigned long identifier, const blink::ResourceResponse &) override final;
    void dispatchDidFinishLoading(blink::DocumentLoader *, unsigned long identifier) override final;
    void dispatchDidLoadResourceFromMemoryCache(const blink::ResourceRequest &, const blink::ResourceResponse &) override final;
    void dispatchDidHandleOnloadEvents(void) override final;
    void dispatchDidReceiveServerRedirectForProvisionalLoad(void) override final;
    void dispatchDidNavigateWithinPage(blink::HistoryItem *, blink::HistoryCommitType) override final;
    void dispatchWillClose(void) override final;
    void dispatchDidStartProvisionalLoad(double triggeringEventTime) override final;
    void dispatchDidReceiveTitle(const String&) override final;
    void dispatchDidChangeIcons(blink::IconType) override final;
    void dispatchDidCommitLoad(blink::HistoryItem*, blink::HistoryCommitType) override final;
    void dispatchDidFailProvisionalLoad(const blink::ResourceError &, blink::HistoryCommitType) override final;
    void dispatchDidFailLoad(const blink::ResourceError &, blink::HistoryCommitType) override final;
    void dispatchDidFinishDocumentLoad(bool documentIsEmpty) override final;
    void dispatchDidFinishLoad(void) override final;
    void dispatchDidChangeThemeColor(void) override final;
    blink::NavigationPolicy decidePolicyForNavigation(const blink::ResourceRequest&, blink::DocumentLoader*, blink::NavigationType, blink::NavigationPolicy, bool shouldReplaceCurrentEntry) override final;
    bool hasPendingNavigation(void) override final;
    void dispatchWillSendSubmitEvent(blink::HTMLFormElement *) override final;
    void dispatchWillSubmitForm(blink::HTMLFormElement *) override final;
    void didStartLoading(blink::LoadStartType) override final;
    void progressEstimateChanged(double progressEstimate) override final;
    void didStopLoading(void) override final;
    void loadURLExternally(const blink::ResourceRequest&, blink::NavigationPolicy, const String& suggestedName, bool replacesCurrentHistoryItem) override final;
    bool navigateBackForward(int offset) const override final;
    void didAccessInitialDocument(void) override final;
    void didDisplayInsecureContent(void) override final;
    void didRunInsecureContent(blink::SecurityOrigin *, const blink::KURL &) override final;
    void didDetectXSS(const blink::KURL &, bool didBlockEntirePage) override final;
    void didDispatchPingLoader(const blink::KURL &) override final;
    void didDisplayContentWithCertificateErrors(const blink::KURL &, const CString& securityInfo, const blink::WebURL& mainResourceUrl, const CString& mainResourceSecurityInfo) override final;
    void didRunContentWithCertificateErrors(const blink::KURL &, const CString& securityInfo, const blink::WebURL& mainResourceUrl, const CString& mainResourceSecurityInfo) override final;
    void didChangePerformanceTiming(void) override final;
    void selectorMatchChanged(const Vector<String>& addedSelectors, const Vector<String>& removedSelectors) override final;
    PassRefPtrWillBeRawPtr<blink::DocumentLoader> createDocumentLoader(blink::LocalFrame *, const blink::ResourceRequest&, const blink::SubstituteData&) override final;
    String doNotTrackValue(void) override final;
    String userAgent(void) override;
    void transitionToCommittedForNewPage(void) override final;
    PassRefPtrWillBeRawPtr<blink::LocalFrame> createFrame(const blink::FrameLoadRequest&, const AtomicString& name, blink::HTMLFrameOwnerElement *) override final;
    bool canCreatePluginWithoutRenderer(const String& mimeType) const override final;
    blink::ObjectContentType objectContentType(const blink::KURL&, const String& mimeType, bool shouldPreferPlugInsForImages) override final;
    void didCreateNewDocument(void) override final;
    void dispatchDidClearWindowObjectInMainWorld(void) override final;
    void documentElementAvailable(void) override final;
    bool allowScriptExtension(const String& extensionName, int extensionGroup, int worldId) override final;
    void didChangeScrollOffset(void) override final;
    void didUpdateCurrentHistoryItem(void) override final;
    void didRemoveAllPendingStylesheet(void) override final;
    bool allowScript(bool enabledPerSettings) override final;
    bool allowScriptFromSource(bool enabledPerSettings, const blink::KURL &) override final;
    bool allowPlugins(bool enabledPerSettings) override final;
    bool allowImage(bool enabledPerSettings, const blink::KURL &) override final;
    bool allowMedia(const blink::KURL &) override final;
    bool allowDisplayingInsecureContent(bool enabledPerSettings, const blink::KURL &) override final;
    bool allowRunningInsecureContent(bool enabledPerSettings, blink::SecurityOrigin *, const blink::KURL &) override final;
    void didNotAllowScript(void) override final;
    void didNotAllowPlugins(void) override final;
    void didUseKeygen(void) override final;
    blink::WebCookieJar* cookieJar(void) const override final;
    void didChangeName(const String&) override final;
    void didEnforceStrictMixedContentChecking(void) override final;
    void didChangeFrameOwnerProperties(blink::HTMLFrameElementBase *) override final;
    void dispatchWillOpenWebSocket(blink::WebSocketHandle *) override final;
    void dispatchWillStartUsingPeerConnectionHandler(blink::WebRTCPeerConnectionHandler *) override final;
    void didRequestAutocomplete(blink::HTMLFormElement*) override final;
    bool allowWebGL(bool enabledPerSettings) override final;
    void didLoseWebGLContext(int) override final;
    void dispatchWillInsertBody(void) override final;
    void dispatchDidChangeResourcePriority(unsigned long identifier, blink::ResourceLoadPriority, int intraPriorityValue) override final;
    bool isControlledByServiceWorker(blink::DocumentLoader&) override final;
    int64_t serviceWorkerID(blink::DocumentLoader&) override final;
    blink::SharedWorkerRepositoryClient* sharedWorkerRepositoryClient(void) override final;
    PassOwnPtr<blink::WebApplicationCacheHost> createApplicationCacheHost(blink::WebApplicationCacheHostClient *) override final;
    void dispatchDidChangeManifest(void) override final;
    void suddenTerminationDisablerChanged(bool present, SuddenTerminationDisablerType) override final;
};

} // namespace BlinKit

#endif // BLINKIT_BLINKIT_FRAME_LOADER_CLIENT_IMPL_H