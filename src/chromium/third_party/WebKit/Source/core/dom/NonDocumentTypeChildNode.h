// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: NonDocumentTypeChildNode.h
// Description: NonDocumentTypeChildNode Class
//      Author: Ziming Li
//     Created: 2019-06-02
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NonDocumentTypeChildNode_h
#define NonDocumentTypeChildNode_h

#include "core/dom/ElementTraversal.h"
#include "core/dom/Node.h"

namespace blink {

class NonDocumentTypeChildNode {
public:
    static Element* previousElementSibling(Node& node)
    {
        return ElementTraversal::previousSibling(node);
    }

    static Element* nextElementSibling(Node& node)
    {
        return ElementTraversal::nextSibling(node);
    }

#ifndef BLINKIT_CRAWLER_ONLY
    static HTMLSlotElement* assignedSlotForBinding(Node& node)
    {
        return node.assignedSlotForBinding();
    }
#endif
};

} // namespace blink

#endif // NonDocumentTypeChildNode_h