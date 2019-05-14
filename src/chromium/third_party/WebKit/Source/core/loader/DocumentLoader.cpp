// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: DocumentLoader.cpp
// Description: DocumentLoader Class
//      Author: Ziming Li
//     Created: 2019-02-12
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/loader/DocumentLoader.h"

#include "core/dom/Document.h"
#include "core/dom/DocumentParser.h"
#include "core/dom/WeakIdentifierMap.h"
#include "core/events/Event.h"
#include "core/fetch/CSSStyleSheetResource.h"
#include "core/fetch/FetchInitiatorTypeNames.h"
#include "core/fetch/FetchRequest.h"
#include "core/fetch/FontResource.h"
#include "core/fetch/ImageResource.h"
#include "core/fetch/MemoryCache.h"
#include "core/fetch/ResourceFetcher.h"
#include "core/fetch/ResourceLoader.h"
#include "core/fetch/ScriptResource.h"
#include "core/frame/FrameHost.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/Settings.h"
#include "core/html/parser/HTMLDocumentParser.h"
#include "core/html/parser/TextResourceDecoder.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/loader/FrameFetchContext.h"
#include "core/loader/FrameLoader.h"
#include "core/loader/FrameLoaderClient.h"
#include "core/loader/LinkLoader.h"
#include "core/page/Page.h"
#include "platform/HTTPNames.h"
#include "platform/Logging.h"
#include "platform/ThreadedDataReceiver.h"
#include "platform/UserGestureIndicator.h"
#include "platform/weborigin/SchemeRegistry.h"
#include "platform/weborigin/SecurityPolicy.h"
#include "public/platform/Platform.h"
#include "public/platform/WebMimeRegistry.h"
#include "wtf/Assertions.h"
#include "wtf/TemporaryChange.h"
#include "wtf/text/WTFString.h"

namespace blink {

static bool isArchiveMIMEType(const String& mimeType)
{
    return equalIgnoringCase("multipart/related", mimeType);
}

DocumentLoader::DocumentLoader(LocalFrame* frame, const ResourceRequest& req, const SubstituteData& substituteData)
    : m_frame(frame)
    , m_fetcher(FrameFetchContext::createContextAndFetcher(this))
    , m_originalRequest(req)
    , m_substituteData(substituteData)
    , m_request(req)
    , m_isClientRedirect(false)
    , m_replacesCurrentHistoryItem(false)
    , m_navigationType(NavigationTypeOther)
    , m_timeOfLastDataReceived(0.0)
    , m_state(NotStarted)
    , m_inDataReceived(false)
    , m_dataBuffer(SharedBuffer::create())
{
}

FrameLoader* DocumentLoader::frameLoader() const
{
    if (!m_frame)
        return nullptr;
    return &m_frame->loader();
}

ResourceLoader* DocumentLoader::mainResourceLoader() const
{
    return m_mainResource ? m_mainResource->loader() : nullptr;
}

DocumentLoader::~DocumentLoader()
{
    ASSERT(!m_frame || !isLoading());
    ASSERT(!m_mainResource);
}

DEFINE_TRACE(DocumentLoader)
{
    visitor->trace(m_frame);
    visitor->trace(m_fetcher);
    // TODO(sof): start tracing ResourcePtr<>s (and m_mainResource.)
    visitor->trace(m_writer);
}

unsigned long DocumentLoader::mainResourceIdentifier() const
{
    return m_mainResource ? m_mainResource->identifier() : 0;
}

Document* DocumentLoader::document() const
{
    if (m_frame && m_frame->loader().documentLoader() == this)
        return m_frame->document();
    return nullptr;
}

const ResourceRequest& DocumentLoader::originalRequest() const
{
    return m_originalRequest;
}

const ResourceRequest& DocumentLoader::request() const
{
    return m_request;
}

const KURL& DocumentLoader::url() const
{
    return m_request.url();
}

void DocumentLoader::startPreload(Resource::Type type, FetchRequest& request)
{
    ResourcePtr<Resource> resource;
    switch (type) {
    case Resource::Image:
#ifdef BLINKIT_CRAWLER_ONLY
        assert(false); // BKTODO: Not reached!
#else
        resource = ImageResource::fetch(request, fetcher());
#endif
        break;
    case Resource::Script:
        resource = ScriptResource::fetch(request, fetcher());
        break;
    case Resource::CSSStyleSheet:
#ifdef BLINKIT_CRAWLER_ONLY
        assert(false); // BKTODO: Not reached!
#else
        resource = CSSStyleSheetResource::fetch(request, fetcher());
#endif
        break;
    case Resource::Font:
#ifdef BLINKIT_CRAWLER_ONLY
        assert(false); // BKTODO: Not reached!
#else
        resource = FontResource::fetch(request, fetcher());
#endif
        break;
    case Resource::Media:
        resource = RawResource::fetchMedia(request, fetcher());
        break;
    case Resource::TextTrack:
        resource = RawResource::fetchTextTrack(request, fetcher());
        break;
    case Resource::ImportResource:
        resource = RawResource::fetchImport(request, fetcher());
        break;
    case Resource::LinkSubresource:
        resource = RawResource::fetch(request, fetcher());
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    if (resource)
        fetcher()->preloadStarted(resource.get());
}

void DocumentLoader::didChangePerformanceTiming()
{
    if (frame() && frame()->isMainFrame() && m_state >= Committed) {
        frameLoader()->client()->didChangePerformanceTiming();
    }
}

void DocumentLoader::updateForSameDocumentNavigation(const KURL& newURL, SameDocumentNavigationSource sameDocumentNavigationSource)
{
    KURL oldURL = m_request.url();
    m_originalRequest.setURL(newURL);
    m_request.setURL(newURL);
    if (sameDocumentNavigationSource == SameDocumentNavigationHistoryApi) {
        m_request.setHTTPMethod(HTTPNames::GET);
        m_request.setHTTPBody(nullptr);
    }
    clearRedirectChain();
    if (m_isClientRedirect)
        appendRedirect(oldURL);
    appendRedirect(newURL);
}

const KURL& DocumentLoader::urlForHistory() const
{
    return unreachableURL().isEmpty() ? url() : unreachableURL();
}

void DocumentLoader::mainReceivedError(const ResourceError& error)
{
    ASSERT(!error.isNull());
    ASSERT(!mainResourceLoader() || !mainResourceLoader()->defersLoading());
    if (!frameLoader())
        return;
    m_state = MainResourceDone;
    frameLoader()->receivedMainResourceError(this, error);
    clearMainResourceHandle();
}

// Cancels the data source's pending loads.  Conceptually, a data source only loads
// one document at a time, but one document may have many related resources.
// stopLoading will stop all loads initiated by the data source,
// but not loads initiated by child frames' data sources -- that's the WebFrame's job.
void DocumentLoader::stopLoading()
{
    RefPtrWillBeRawPtr<LocalFrame> protectFrame(m_frame.get());
    RefPtrWillBeRawPtr<DocumentLoader> protectLoader(this);

    if (isLoading())
        cancelMainResourceLoad(ResourceError::cancelledError(m_request.url()));
    m_fetcher->stopFetching();
}

void DocumentLoader::commitIfReady()
{
    if (m_state < Committed) {
        m_state = Committed;
        frameLoader()->commitProvisionalLoad();
    }
}

bool DocumentLoader::isLoading() const
{
    if (document() && document()->hasActiveParser())
        return true;

    return (m_state > NotStarted && m_state < MainResourceDone) || m_fetcher->isFetching();
}

void DocumentLoader::notifyFinished(Resource* resource)
{
    ASSERT_UNUSED(resource, m_mainResource == resource);
    ASSERT(m_mainResource);

    RefPtrWillBeRawPtr<DocumentLoader> protect(this);

    if (!m_mainResource->errorOccurred() && !m_mainResource->wasCanceled()) {
        finishedLoading(m_mainResource->loadFinishTime());
        return;
    }

    mainReceivedError(m_mainResource->resourceError());
}

void DocumentLoader::finishedLoading(double finishTime)
{
    ASSERT(!mainResourceLoader() || !mainResourceLoader()->defersLoading());

    RefPtrWillBeRawPtr<DocumentLoader> protect(this);

    double responseEndTime = finishTime;
    if (!responseEndTime)
        responseEndTime = m_timeOfLastDataReceived;
    if (!responseEndTime)
        responseEndTime = monotonicallyIncreasingTime();

    commitIfReady();
    if (!frameLoader())
        return;

    if (!maybeCreateArchive()) {
        // If this is an empty document, it will not have actually been created yet. Commit dummy data so that
        // DocumentWriter::begin() gets called and creates the Document.
        if (!m_writer)
            commitData(0, 0);
    }

    endWriting(m_writer.get());
    if (m_state < MainResourceDone)
        m_state = MainResourceDone;
    clearMainResourceHandle();
}

bool DocumentLoader::isRedirectAfterPost(const ResourceRequest& newRequest, const ResourceResponse& redirectResponse)
{
    int status = redirectResponse.httpStatusCode();
    if (((status >= 301 && status <= 303) || status == 307)
        && m_originalRequest.httpMethod() == HTTPNames::POST)
        return true;

    return false;
}

void DocumentLoader::redirectReceived(Resource* resource, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    ASSERT_UNUSED(resource, resource == m_mainResource);
    willSendRequest(request, redirectResponse);
}

void DocumentLoader::updateRequest(Resource* resource, const ResourceRequest& request)
{
    ASSERT_UNUSED(resource, resource == m_mainResource);
    m_request = request;
}

static bool isFormSubmission(NavigationType type)
{
    return type == NavigationTypeFormSubmitted || type == NavigationTypeFormResubmitted;
}

void DocumentLoader::willSendRequest(ResourceRequest& newRequest, const ResourceResponse& redirectResponse)
{
    // Note that there are no asserts here as there are for the other callbacks. This is due to the
    // fact that this "callback" is sent when starting every load, and the state of callback
    // deferrals plays less of a part in this function in preventing the bad behavior deferring
    // callbacks is meant to prevent.
    ASSERT(!newRequest.isNull());

    if (!redirectResponse.isNull()) {
        // If the redirecting url is not allowed to display content from the target origin,
        // then block the redirect.
        RefPtr<SecurityOrigin> redirectingOrigin = SecurityOrigin::create(redirectResponse.url());
        if (!redirectingOrigin->canDisplay(newRequest.url())) {
            FrameLoader::reportLocalLoadFailed(m_frame, newRequest.url().string());
            cancelMainResourceLoad(ResourceError::cancelledError(newRequest.url()));
            return;
        }
    }

    // If we're fielding a redirect in response to a POST, force a load from origin, since
    // this is a common site technique to return to a page viewing some data that the POST
    // just modified.
    if (newRequest.cachePolicy() == UseProtocolCachePolicy && isRedirectAfterPost(newRequest, redirectResponse))
        newRequest.setCachePolicy(ReloadBypassingCache);

    m_request = newRequest;

    if (redirectResponse.isNull())
        return;

    appendRedirect(newRequest.url());
    frameLoader()->receivedMainResourceRedirect(m_request.url());
    if (!frameLoader()->shouldContinueForNavigationPolicy(newRequest, SubstituteData(), this, CheckContentSecurityPolicy, m_navigationType, NavigationPolicyCurrentTab, replacesCurrentHistoryItem()))
        cancelMainResourceLoad(ResourceError::cancelledError(m_request.url()));
}

static bool canShowMIMEType(const String& mimeType)
{
    return Platform::current()->mimeRegistry()->supportsMIMEType(mimeType) == WebMimeRegistry::IsSupported;
}

bool DocumentLoader::shouldContinueForResponse() const
{
    if (m_substituteData.isValid())
        return true;

    int statusCode = m_response.httpStatusCode();
    if (statusCode == 204 || statusCode == 205) {
        // The server does not want us to replace the page contents.
        return false;
    }

    if (contentDispositionType(m_response.httpHeaderField(HTTPNames::Content_Disposition)) == ContentDispositionAttachment) {
        // The server wants us to download instead of replacing the page contents.
        // Downloading is handled by the embedder, but we still get the initial
        // response so that we can ignore it and clean up properly.
        return false;
    }

    if (!canShowMIMEType(m_response.mimeType()))
        return false;

    // Prevent remote web archives from loading because they can claim to be from any domain and thus avoid cross-domain security checks.
    if (isArchiveMIMEType(m_response.mimeType()) && !SchemeRegistry::shouldTreatURLSchemeAsLocal(m_request.url().protocol()))
        return false;

    return true;
}

void DocumentLoader::cancelLoadAfterXFrameOptionsOrCSPDenied(const ResourceResponse& response)
{
    // The load event might have detached this frame. In that case, the load will already have been cancelled during detach.
    if (frameLoader())
        cancelMainResourceLoad(ResourceError::cancelledError(m_request.url()));
    return;
}

void DocumentLoader::responseReceived(Resource* resource, const ResourceResponse& response, PassOwnPtr<WebDataConsumerHandle> handle)
{
    ASSERT_UNUSED(resource, m_mainResource == resource);
    ASSERT_UNUSED(handle, !handle);
    RefPtrWillBeRawPtr<DocumentLoader> protect(this);
    ASSERT(frame());

    // The memory cache doesn't understand the application cache or its caching rules. So if a main resource is served
    // from the application cache, ensure we don't save the result for future use. All responses loaded
    // from appcache will have a non-zero appCacheID().
    if (response.appCacheID())
        memoryCache()->remove(m_mainResource.get());

#if 0 // BKTODO: Remove CSP.
    m_contentSecurityPolicy = ContentSecurityPolicy::create();
    m_contentSecurityPolicy->setOverrideURLForSelf(response.url());
    m_contentSecurityPolicy->didReceiveHeaders(ContentSecurityPolicyResponseHeaders(response));
    if (!m_contentSecurityPolicy->allowAncestors(m_frame, response.url())) {
        cancelLoadAfterXFrameOptionsOrCSPDenied(response);
        return;
    }

    // 'frame-ancestors' obviates 'x-frame-options': https://w3c.github.io/webappsec/specs/content-security-policy/#frame-ancestors-and-frame-options
    if (!m_contentSecurityPolicy->isFrameAncestorsEnforced()) {
        HTTPHeaderMap::const_iterator it = response.httpHeaderFields().find(HTTPNames::X_Frame_Options);
        if (it != response.httpHeaderFields().end()) {
            String content = it->value;
            if (frameLoader()->shouldInterruptLoadForXFrameOptions(content, response.url(), mainResourceIdentifier())) {
                String message = "Refused to display '" + response.url().elidedString() + "' in a frame because it set 'X-Frame-Options' to '" + content + "'.";
                RefPtrWillBeRawPtr<ConsoleMessage> consoleMessage = ConsoleMessage::create(SecurityMessageSource, ErrorMessageLevel, message);
                consoleMessage->setRequestIdentifier(mainResourceIdentifier());
                frame()->document()->addConsoleMessage(consoleMessage.release());

                cancelLoadAfterXFrameOptionsOrCSPDenied(response);
                return;
            }
        }
    }
#endif

    ASSERT(!mainResourceLoader() || !mainResourceLoader()->defersLoading());

    m_response = response;

    if (isArchiveMIMEType(m_response.mimeType()) && m_mainResource->dataBufferingPolicy() != BufferData)
        m_mainResource->setDataBufferingPolicy(BufferData);

    if (!shouldContinueForResponse())
        cancelMainResourceLoad(ResourceError::cancelledError(m_request.url()));
}

const AtomicString& DocumentLoader::Encoding(void)
{
#ifndef BLINKIT_CRAWLER_ONLY
    if (!m_frame->IsCrawlerFrame())
    {
        const AtomicString &overrideEncoding = m_frame->host()->overrideEncoding();
        if (!overrideEncoding.isNull())
            return overrideEncoding;
    }
#endif
    return response().textEncodingName();
}

void DocumentLoader::ensureWriter(const AtomicString& mimeType, const KURL& overridingURL)
{
    if (m_writer)
        return;

    const AtomicString& encoding = Encoding();

    // Prepare a DocumentInit before clearing the frame, because it may need to
    // inherit an aliased security context.
    DocumentInit init(url(), m_frame);
    m_frame->loader().clear();
    ASSERT(m_frame->IsCrawlerFrame() || nullptr != m_frame->page());

    ParserSynchronizationPolicy parsingPolicy = AllowAsynchronousParsing;
    if ((m_substituteData.isValid() && m_substituteData.forceSynchronousLoad()))
        parsingPolicy = ForceSynchronousParsing;

    m_writer = createWriterFor(0, init, mimeType, encoding, false, parsingPolicy);
    m_writer->setDocumentWasLoadedAsPartOfNavigation();

    // This should be set before receivedFirstData().
    if (!overridingURL.isEmpty())
        m_frame->document()->setBaseURLOverride(overridingURL);

    // Call receivedFirstData() exactly once per load.
    frameLoader()->receivedFirstData();
    m_frame->document()->maybeHandleHttpRefresh(m_response.httpHeaderField(HTTPNames::Refresh), Document::HttpRefreshFromHeader);
}

void DocumentLoader::commitData(const char* bytes, size_t length)
{
    ASSERT(m_state < MainResourceDone);
    ensureWriter(m_response.mimeType());

    // This can happen if document.close() is called by an event handler while
    // there's still pending incoming data.
    if (m_frame && !m_frame->document()->parsing()) {
        cancelMainResourceLoad(ResourceError::cancelledError(m_request.url()));
        return;
    }

    if (length)
        m_state = DataReceived;

    m_writer->addData(bytes, length);
}

void DocumentLoader::dataReceived(Resource* resource, const char* data, size_t length)
{
    ASSERT(data);
    ASSERT(length);
    ASSERT_UNUSED(resource, resource == m_mainResource);
    ASSERT(!m_response.isNull());
    ASSERT(!mainResourceLoader() || !mainResourceLoader()->defersLoading());

    if (m_inDataReceived) {
        // If this function is reentered, defer processing of the additional
        // data to the top-level invocation. Reentrant calls can occur because
        // of web platform (mis-)features that require running a nested message
        // loop:
        // - alert(), confirm(), prompt()
        // - Detach of plugin elements.
        // - Synchronous XMLHTTPRequest
        m_dataBuffer->append(data, length);
        return;
    }

    // Both unloading the old page and parsing the new page may execute JavaScript which destroys the datasource
    // by starting a new load, so retain temporarily.
    RefPtrWillBeRawPtr<LocalFrame> protectFrame(m_frame.get());
    RefPtrWillBeRawPtr<DocumentLoader> protectLoader(this);

    TemporaryChange<bool> reentrancyProtector(m_inDataReceived, true);
    processData(data, length);

    // Process data received in reentrant invocations. Note that the
    // invocations of processData() may queue more data in reentrant
    // invocations, so iterate until it's empty.
    const char* segment;
    size_t pos = 0;
    while (size_t length = m_dataBuffer->getSomeData(segment, pos)) {
        processData(segment, length);
        pos += length;
    }
    // All data has been consumed, so flush the buffer.
    m_dataBuffer->clear();
}

void DocumentLoader::processData(const char* data, size_t length)
{
    m_timeOfLastDataReceived = monotonicallyIncreasingTime();

    if (isArchiveMIMEType(response().mimeType()))
        return;
    commitIfReady();
    if (!frameLoader())
        return;
    commitData(data, length);

    // If we are sending data to MediaDocument, we should stop here
    // and cancel the request.
    if (m_frame && m_frame->document()->isMediaDocument())
        cancelMainResourceLoad(ResourceError::cancelledError(m_request.url()));
}

void DocumentLoader::clearRedirectChain()
{
    m_redirectChain.clear();
}

void DocumentLoader::appendRedirect(const KURL& url)
{
    m_redirectChain.append(url);
}

bool DocumentLoader::loadingMultipartContent() const
{
    return mainResourceLoader() ? mainResourceLoader()->loadingMultipartContent() : false;
}

void DocumentLoader::detachFromFrame()
{
    ASSERT(m_frame);
    RefPtrWillBeRawPtr<LocalFrame> protectFrame(m_frame.get());
    RefPtrWillBeRawPtr<DocumentLoader> protectLoader(this);

    // It never makes sense to have a document loader that is detached from its
    // frame have any loads active, so go ahead and kill all the loads.
    stopLoading();

    // If that load cancellation triggered another detach, leave.
    // (fast/frames/detach-frame-nested-no-crash.html is an example of this.)
    if (!m_frame)
        return;

    m_fetcher->clearContext();

    WeakIdentifierMap<DocumentLoader>::notifyObjectDestroyed(this);
    clearMainResourceHandle();
    m_frame = nullptr;
}

void DocumentLoader::clearMainResourceHandle()
{
    if (!m_mainResource)
        return;
    m_mainResource->removeClient(this);
    m_mainResource = nullptr;
}

bool DocumentLoader::maybeCreateArchive()
{
    return false; // MHTML is not supported in BlinKit.
}

const AtomicString& DocumentLoader::responseMIMEType() const
{
    return m_response.mimeType();
}

const KURL& DocumentLoader::unreachableURL() const
{
    return m_substituteData.failingURL();
}

void DocumentLoader::setDefersLoading(bool defers)
{
    // Multiple frames may be loading the same main resource simultaneously. If deferral state changes,
    // each frame's DocumentLoader will try to send a setDefersLoading() to the same underlying ResourceLoader. Ensure only
    // the "owning" DocumentLoader does so, as setDefersLoading() is not resilient to setting the same value repeatedly.
    if (mainResourceLoader() && mainResourceLoader()->isLoadedBy(m_fetcher.get()))
        mainResourceLoader()->setDefersLoading(defers);

    m_fetcher->setDefersLoading(defers);
}

bool DocumentLoader::maybeLoadEmpty()
{
    bool shouldLoadEmpty = !m_substituteData.isValid() && (m_request.url().isEmpty() || SchemeRegistry::shouldLoadURLSchemeAsEmptyDocument(m_request.url().protocol()));
    if (!shouldLoadEmpty)
        return false;

    if (m_request.url().isEmpty() && !frameLoader()->stateMachine()->creatingInitialEmptyDocument())
        m_request.setURL(blankURL());
    m_response = ResourceResponse(m_request.url(), "text/html", 0, nullAtom, String());
    finishedLoading(monotonicallyIncreasingTime());
    return true;
}

void DocumentLoader::startLoadingMainResource()
{
    RefPtrWillBeRawPtr<DocumentLoader> protect(this);
    if (!m_referenceMonotonicTime)
        m_referenceMonotonicTime = monotonicallyIncreasingTime();
    ASSERT(!m_mainResource);
    ASSERT(m_state == NotStarted);
    m_state = Provisional;

    if (maybeLoadEmpty())
        return;

    willSendRequest(m_request, ResourceResponse());

    // willSendRequest() may lead to our LocalFrame being detached or cancelling the load via nulling the ResourceRequest.
    if (!m_frame || m_request.isNull())
        return;

    ResourceRequest request(m_request);
    DEFINE_STATIC_LOCAL(ResourceLoaderOptions, mainResourceLoadOptions,
        (DoNotBufferData, AllowStoredCredentials, ClientRequestedCredentials, CheckContentSecurityPolicy, DocumentContext));
    FetchRequest cachedResourceRequest(request, FetchInitiatorTypeNames::document, mainResourceLoadOptions);
    m_mainResource = RawResource::fetchMainResource(cachedResourceRequest, fetcher(), m_substituteData);
    if (!m_mainResource) {
        m_request = ResourceRequest();
        maybeLoadEmpty();
        return;
    }
    m_mainResource->addClient(this);

    // A bunch of headers are set when the underlying ResourceLoader is created, and m_request needs to include those.
    if (mainResourceLoader())
        request = mainResourceLoader()->originalRequest();
    // If there was a fragment identifier on m_request, the cache will have stripped it. m_request should include
    // the fragment identifier, so add that back in.
    if (equalIgnoringFragmentIdentifier(m_request.url(), request.url()))
        request.setURL(m_request.url());
    m_request = request;
}

void DocumentLoader::cancelMainResourceLoad(const ResourceError& resourceError)
{
    RefPtrWillBeRawPtr<DocumentLoader> protect(this);
    ResourceError error = resourceError.isNull() ? ResourceError::cancelledError(m_request.url()) : resourceError;

    if (mainResourceLoader())
        mainResourceLoader()->cancel(error);

    mainReceivedError(error);
}

void DocumentLoader::attachThreadedDataReceiver(PassRefPtrWillBeRawPtr<ThreadedDataReceiver> threadedDataReceiver)
{
    if (mainResourceLoader())
        mainResourceLoader()->attachThreadedDataReceiver(threadedDataReceiver);
}

void DocumentLoader::acceptDataFromThreadedReceiver(const char* data, int dataLength, int encodedDataLength)
{
    m_fetcher->acceptDataFromThreadedReceiver(mainResourceIdentifier(), data, dataLength, encodedDataLength);
}

void DocumentLoader::endWriting(DocumentWriter* writer)
{
    ASSERT_UNUSED(writer, m_writer == writer);
    m_writer->end();
    m_writer.clear();
}

PassRefPtrWillBeRawPtr<DocumentWriter> DocumentLoader::createWriterFor(const Document* ownerDocument, const DocumentInit& init, const AtomicString& mimeType, const AtomicString& encoding, bool dispatch, ParserSynchronizationPolicy parsingPolicy)
{
    LocalFrame* frame = init.frame();

    ASSERT(!frame->document() || !frame->document()->isActive());

    if (!init.shouldReuseDefaultView())
        frame->setDOMWindow(LocalDOMWindow::create(*frame));

    RefPtrWillBeRawPtr<Document> document = frame->localDOMWindow()->installNewDocument(mimeType, init);
    if (ownerDocument) {
        assert(false); // BKTODO: Remove ownerDocument!
#if 0
        document->setCookieURL(ownerDocument->cookieURL());
        document->updateSecurityOrigin(ownerDocument->securityOrigin());
#endif
    }

    frame->loader().didBeginDocument(dispatch);

    return DocumentWriter::create(document.get(), parsingPolicy, mimeType, encoding);
}

const AtomicString& DocumentLoader::mimeType() const
{
    if (m_writer)
        return m_writer->mimeType();
    return m_response.mimeType();
}

DEFINE_WEAK_IDENTIFIER_MAP(DocumentLoader);

} // namespace blink
