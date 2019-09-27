// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: ConsoleBase.h
// Description: ConsoleBase Class
//      Author: Ziming Li
//     Created: 2019-02-14
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef ConsoleBase_h
#define ConsoleBase_h

#include "bindings/core/v8/ScriptWrappable.h"
#include "core/frame/ConsoleTypes.h"
#include "core/frame/DOMWindowProperty.h"
#include "platform/heap/Handle.h"
#include "wtf/Forward.h"
#include "wtf/text/WTFString.h"

namespace blink {

class ConsoleMessage;
class ExecutionContext;

class ConsoleBase : public GarbageCollectedFinalized<ConsoleBase>, public ScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
public:
    virtual ~ConsoleBase();

    DEFINE_INLINE_VIRTUAL_TRACE() { }

protected:
    virtual ExecutionContext* context() = 0;
    virtual void reportMessageToConsole(PassRefPtrWillBeRawPtr<ConsoleMessage>) = 0;

private:
    HashCountedSet<String> m_counts;
    HashMap<String, double> m_times;
};

} // namespace blink

#endif // ConsoleBase_h