// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: http_names.h
// Description: HTTP Names
//      Author: Ziming Li
//     Created: 2019-10-11
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

#ifndef BLINKIT_BLINK_HTTP_NAMES_H
#define BLINKIT_BLINK_HTTP_NAMES_H

#pragma once

#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {
namespace http_names {

extern const WTF::AtomicString &kContentDisposition;
extern const WTF::AtomicString &kContentLanguage;
extern const WTF::AtomicString &kContentType;
extern const WTF::AtomicString &kGET;
extern const WTF::AtomicString &kReferer;
extern const WTF::AtomicString &kRefresh;
extern const WTF::AtomicString &kUserAgent;

constexpr unsigned kNamesCount = 7;

void Init(void);

} // namespace http_names
} // namespace blink

#endif // BLINKIT_BLINK_HTTP_NAMES_H
