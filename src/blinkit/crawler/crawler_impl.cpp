// -------------------------------------------------
// BlinKit - BlinKit Library
// -------------------------------------------------
//   File Name: crawler_impl.cpp
// Description: CrawlerImpl Class
//      Author: Ziming Li
//     Created: 2019-03-20
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

#include "crawler_impl.h"

#include "blinkit/misc/controller_impl.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/loader/frame_load_request.h"
#include "url/bk_url.h"
#if 0 // BKTODO:
#include "platform/network/ResourceError.h"

#include "app/app_impl.h"
#include "blink_impl/cookie_jar_impl.h"

#include "js/public/script_controller.h"
#endif // 0

using namespace blink;
using namespace BlinKit;

CrawlerImpl::CrawlerImpl(const BkCrawlerClient &client) : m_client(client), m_frame(LocalFrame::Create(*this))
{
    m_frame->Init();
}

CrawlerImpl::~CrawlerImpl(void)
{
#if 0 // BKTODO:
    m_frame->detach(FrameDetachType::Remove);
#endif
}

void CrawlerImpl::DispatchDidFinishLoad(void)
{
    m_client.DocumentReady(m_client.UserData);
}

void CrawlerImpl::ProcessRequestComplete(BkResponse response, BkWorkController controller)
{
    if (nullptr != m_client.RequestComplete)
        m_client.RequestComplete(response, controller, m_client.UserData);
    else
        controller->ContinueWorking();
}

#if 0 // BKTODO:
int BKAPI CrawlerImpl::AccessCrawlerMember(const char *name, BkCallback &callback)
{
    return m_frame->script().AccessCrawlerMember(name, callback);
}

int BKAPI CrawlerImpl::CallCrawler(const char *method, BkCallback *callback)
{
    return m_frame->script().CallCrawler(method, callback);
}

int BKAPI CrawlerImpl::CallFunction(const char *name, BkCallback *callback)
{
    return m_frame->script().CallFunction(name, callback);
}

void CrawlerImpl::CancelLoading(void)
{
    m_frame->loader().stopAllLoaders();
}

void CrawlerImpl::dispatchDidFailProvisionalLoad(const ResourceError &error, HistoryCommitType)
{
    m_client.LoadFailed(error.errorCode(), this);
}

void CrawlerImpl::dispatchDidFinishLoad(void)
{
    m_client.DocumentReady(this);
}

int BKAPI CrawlerImpl::Eval(const char *code, size_t length, BkCallback *callback)
{
    return m_frame->script().Eval(code, length, callback);
}

std::string CrawlerImpl::GetCookies(const std::string &URL) const
{
    std::string ret;
    m_client.GetCookies(URL.c_str(), BkMakeBuffer(ret).Wrap());
    if (ret.empty())
        ret = AppImpl::Get().CookieJar().GetCookies(URL);
    return ret;
}

std::tuple<int, std::string> CrawlerImpl::Initialize(void)
{
    std::string script;
    m_client.GetUserScript(BkMakeBuffer(script).Wrap());
    return m_frame->script().CreateCrawlerObject(script.data(), script.length());
}

int BKAPI CrawlerImpl::RegisterCrawlerFunction(const char *name, BkCallback &functionImpl)
{
    return m_frame->script().RegisterFunction(name, functionImpl);
}
#endif // 0

int CrawlerImpl::Run(const char *URL)
{
    BkURL u(URL);
    if (!u.SchemeIsHTTPOrHTTPS())
    {
        assert(u.SchemeIsHTTPOrHTTPS());
        return BK_ERR_URI;
    }

    FrameLoadRequest request(nullptr, ResourceRequest(u));
    request.GetResourceRequest().SetCrawler(this);
    m_frame->Loader().StartNavigation(request);
    return BK_ERR_SUCCESS;
}

void CrawlerImpl::TransitionToCommittedForNewPage(void)
{
    // Nothing to do for crawlers.
}

String CrawlerImpl::UserAgent(void)
{
    BKLOG("// BKTODO: Get user agent string from crawler object.");
    return LocalFrameClientImpl::UserAgent();
}

#if 0 // BKTODO:
String CrawlerImpl::userAgent(void)
{
    std::string userAgent;
    const auto callback = [&userAgent](const BkValue &prop)
    {
        prop.GetAsString(BkMakeBuffer(userAgent).Wrap());
    };
    m_frame->script().GetCrawlerProperty("userAgent", callback);

    if (userAgent.empty())
        m_client.GetUserAgent(BkMakeBuffer(userAgent).Wrap());
    if (!userAgent.empty())
        return String::fromUTF8(userAgent.data(), userAgent.length());

    return FrameLoaderClientImpl::userAgent();
}
#endif // 0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" {

BKEXPORT BkCrawler BKAPI BkCreateCrawler(BkCrawlerClient *client)
{
    return new CrawlerImpl(*client);
}

BKEXPORT void BKAPI BkDestroyCrawler(BkCrawler crawler)
{
    delete crawler;
}

BKEXPORT int BKAPI BkRunCrawler(BkCrawler crawler, const char *URL)
{
    return crawler->Run(URL);
}

} // extern "C"
