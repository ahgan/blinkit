// -------------------------------------------------
// BlinKit - BlinKit Library
// -------------------------------------------------
//   File Name: http_loader_task.h
// Description: HTTPLoaderTask Class
//      Author: Ziming Li
//     Created: 2019-03-21
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

#ifndef BLINKIT_BLINKIT_HTTP_LOADER_TASK_H
#define BLINKIT_BLINKIT_HTTP_LOADER_TASK_H

#pragma once

#include "sdk/include/BlinKit.h"
#include "public/platform/WebURLRequest.h"
#include "loader_task.h"

namespace BlinKit {

class CrawlerImpl;

class HTTPLoaderTask final : public LoaderTask
{
public:
    HTTPLoaderTask(CrawlerImpl &crawler, const blink::WebURLRequest &request, blink::WebURLLoaderClient *client);
private:
    int LoadRemoteData(void);

    // blink::WebTaskRunner::Task
    void run(void) override;

    CrawlerImpl &m_crawler;
    std::string m_method;
    std::unordered_map<std::string, std::string> m_headers;
};

} // namespace BlinKit

#endif // BLINKIT_BLINKIT_HTTP_LOADER_TASK_H