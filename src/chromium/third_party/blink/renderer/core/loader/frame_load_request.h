// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: frame_load_request.h
// Description: FrameLoadRequest Class
//      Author: Ziming Li
//     Created: 2019-09-14
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2003, 2006, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BLINKIT_BLINK_FRAME_LOAD_REQUEST_H
#define BLINKIT_BLINK_FRAME_LOAD_REQUEST_H

#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class Document;
class Element;

struct FrameLoadRequest
{
public:
    FrameLoadRequest(Document *originDocument, const ResourceRequest &resourceRequest);

    Document* OriginDocument(void) const { return m_originDocument.Get(); }

    ResourceRequest& GetResourceRequest(void) { return m_resourceRequest; }
    const ResourceRequest& GetResourceRequest(void) const { return m_resourceRequest; }

    Element* Form(void) const { return m_form.Get(); }
private:
    Member<Document> m_originDocument;
    ResourceRequest m_resourceRequest;
    Member<Element> m_form;
};

}  // namespace blink

#endif  // BLINKIT_BLINK_FRAME_LOAD_REQUEST_H
