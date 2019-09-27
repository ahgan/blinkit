// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: WindowNameCollection.cpp
// Description: WindowNameCollection Class
//      Author: Ziming Li
//     Created: 2019-02-09
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/WindowNameCollection.h"

#include "core/html/HTMLImageElement.h"

namespace blink {

WindowNameCollection::WindowNameCollection(ContainerNode& document, const AtomicString& name)
    : HTMLNameCollection(document, WindowNamedItems, name)
{
}

bool WindowNameCollection::elementMatches(const Element& element) const
{
#ifdef BLINKIT_CRAWLER_ONLY
    assert(false); // BKTODO: Not reached!
    return false;
#else
    // Match only images, forms, embeds and objects by name,
    // but anything by id
    if (isHTMLImageElement(element)
        || isHTMLFormElement(element)) {
        if (element.getNameAttribute() == m_name)
            return true;
    }
    return element.getIdAttribute() == m_name;
#endif
}

} // namespace blink