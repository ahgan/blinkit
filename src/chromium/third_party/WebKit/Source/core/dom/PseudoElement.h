// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: PseudoElement.h
// Description: PseudoElement Class
//      Author: Ziming Li
//     Created: 2019-04-02
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PseudoElement_h
#define PseudoElement_h

#include "core/CoreExport.h"
#include "core/dom/Element.h"
#include "core/style/ComputedStyle.h"

namespace blink {

class CORE_EXPORT PseudoElement : public Element {
public:
    static PassRefPtrWillBeRawPtr<PseudoElement> create(Element* parent, PseudoId);

#ifndef BLINKIT_CRAWLER_ONLY
    PassRefPtr<ComputedStyle> customStyleForLayoutObject() override;
#endif
    void attach(const AttachContext& = AttachContext()) override;
#ifndef BLINKIT_CRAWLER_ONLY
    bool layoutObjectIsNeeded(const ComputedStyle&) override;

    bool canStartSelection() const override { return false; }
#endif
    bool canContainRangeEndPoint() const override { return false; }
    PseudoId pseudoId() const override { return m_pseudoId; }

    static String pseudoElementNameForEvents(PseudoId);

    Node* findAssociatedNode() const;

    virtual void dispose();

protected:
    PseudoElement(Element*, PseudoId);

private:
    void didRecalcStyle(StyleRecalcChange) override;

    PseudoId m_pseudoId;
};

const QualifiedName& pseudoElementTagName();

inline bool pseudoElementLayoutObjectIsNeeded(const ComputedStyle* style)
{
#ifdef BLINKIT_CRAWLER_ONLY
    assert(false); // BKTODO: Not reached!
    return false;
#else
    if (!style)
        return false;
    if (style->display() == NONE)
        return false;
    if (style->styleType() == FIRST_LETTER || style->styleType() == BACKDROP)
        return true;
    return style->contentData();
#endif
}

DEFINE_ELEMENT_TYPE_CASTS(PseudoElement, isPseudoElement());

} // namespace

#endif