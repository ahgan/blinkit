// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: frame_loader.h
// Description: FrameLoader Class
//      Author: Ziming Li
//     Created: 2019-09-14
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) Research In Motion Limited 2009. All rights reserved.
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

#ifndef BLINKIT_BLINK_FRAME_LOADER_H
#define BLINKIT_BLINK_FRAME_LOADER_H

#pragma once

#include "third_party/blink/public/web/web_frame_load_type.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class DocumentLoader;
class FrameLoaderStateMachine;
struct FrameLoadRequest;
class LocalFrame;
class LocalFrameClient;

class FrameLoader final
{
    DISALLOW_NEW();
public:
    explicit FrameLoader(LocalFrame *frame);
    ~FrameLoader(void);

    FrameLoaderStateMachine* StateMachine(void) const { return m_stateMachine.get(); }

    void Init(void);
    void StartNavigation(const FrameLoadRequest &request, WebFrameLoadType loadType = WebFrameLoadType::kStandard);
private:
    LocalFrameClient* Client(void) const;

    Member<LocalFrame> m_frame;
    std::unique_ptr<FrameLoaderStateMachine> m_stateMachine;
    std::unique_ptr<DocumentLoader> m_provisionalDocumentLoader;

    DISALLOW_COPY_AND_ASSIGN(FrameLoader);
};

}  // namespace blink

#endif  // BLINKIT_BLINK_FRAME_LOADER_H