// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: tree_scope.cpp
// Description: TreeScope Class
//      Author: Ziming Li
//     Created: 2019-10-19
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tree_scope.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/tree_scope_adopter.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"

namespace blink {

TreeScope::TreeScope(ContainerNode &rootNode, Document &document)
    : m_rootNode(&rootNode)
    , m_document(&document)
    , m_parentTreeScope(&document)
{
    ASSERT(rootNode != document);
    m_rootNode->SetTreeScope(this);
}

TreeScope::TreeScope(Document &document)
    : m_rootNode(&document)
    , m_document(&document)
{
    m_rootNode->SetTreeScope(this);
}

TreeScope::~TreeScope(void) = default;

void TreeScope::AdoptIfNeeded(Node &node)
{
    // Script is forbidden to protect against event handlers firing in the middle
    // of rescoping in |didMoveToNewDocument| callbacks. See
    // https://crbug.com/605766 and https://crbug.com/606651.
    ScriptForbiddenScope forbidScript;
    ASSERT(nullptr != this);
    ASSERT(!node.IsDocumentNode());
    TreeScopeAdopter adopter(node, *this);
    if (adopter.NeedsScopeChange())
        adopter.Execute();
}

bool TreeScope::IsInclusiveOlderSiblingShadowRootOrAncestorTreeScopeOf(const TreeScope &scope) const
{
    for (const TreeScope *current = &scope; nullptr != current; current = current->ParentTreeScope())
    {
        if (current == this)
            return true;
    }
    return false;
}

}  // namespace blink
