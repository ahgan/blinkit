// -------------------------------------------------
// BlinKit - BlinKit Library
// -------------------------------------------------
//   File Name: context_impl.h
// Description: ContextImpl Class
//      Author: Ziming Li
//     Created: 2020-01-21
// -------------------------------------------------
// Copyright (C) 2020 MingYang Software Technology.
// -------------------------------------------------

#ifndef BLINKIT_BLINKIT_CONTEXT_IMPL_H
#define BLINKIT_BLINKIT_CONTEXT_IMPL_H

#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include "bk_js.h"
#include "duktape/duktape.h"

namespace blink {
class ExecutionContext;
class LocalFrame;
}

namespace BlinKit {
class GCPool;
}

class CrawlerImpl;

class ContextImpl final
{
public:
    ContextImpl(const blink::LocalFrame &frame);
    ~ContextImpl(void);

    static ContextImpl* From(duk_context *ctx);
    static ContextImpl* From(blink::ExecutionContext *executionContext);

    void Reset(void);

    const char* LookupPrototypeName(const std::string &tagName) const;

    typedef std::function<void(duk_context *)> Callback;
    bool AccessCrawler(const Callback &worker);
    void Eval(const std::string_view code, const Callback &callback, const char *fileName = "eval");
    void ConsoleOutput(int type, const char *msg) { m_consoleMessager(type, msg); }

    BlinKit::GCPool& GetGCPool(void);
    duk_context* GetRawContext(void) const { return m_ctx; }
private:
    void InitializeHeapStash(void);
    static void RegisterPrototypesForCrawler(duk_context *ctx);
    void CreateCrawlerObject(const CrawlerImpl &crawler);
    static void ExposeGlobals(duk_context *ctx, duk_idx_t dst);

    const blink::LocalFrame &m_frame;
    duk_context *m_ctx;
    std::function<void(int, const char *)> m_consoleMessager;
    const std::unordered_map<std::string, const char *> &m_prototypeMap;
};

#endif // BLINKIT_BLINKIT_CONTEXT_IMPL_H
