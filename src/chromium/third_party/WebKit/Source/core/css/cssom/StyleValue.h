// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: StyleValue.h
// Description: StyleValue Class
//      Author: Ziming Li
//     Created: 2019-02-11
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef StyleValue_h
#define StyleValue_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "core/CoreExport.h"
#include "core/css/CSSValue.h"
#include "core/css/CSSValuePool.h"

namespace blink {

class ScriptState;
class ScriptValue;

class CORE_EXPORT StyleValue : public GarbageCollectedFinalized<StyleValue>, public ScriptWrappable {
    WTF_MAKE_NONCOPYABLE(StyleValue);
    DEFINE_WRAPPERTYPEINFO();
public:
    enum StyleValueType {
        KeywordValueType, SimpleLengthType, CalcLengthType, NumberType, TransformValueType
    };

    virtual ~StyleValue() { }

    virtual StyleValueType type() const = 0;

    static StyleValue* create(const CSSValue&);

    virtual String cssString() const = 0;
    virtual PassRefPtrWillBeRawPtr<CSSValue> toCSSValue() const = 0;

    DEFINE_INLINE_VIRTUAL_TRACE() { }

protected:
    StyleValue() {}
};

} // namespace blink

#endif
