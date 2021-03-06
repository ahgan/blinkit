// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: comment.h
// Description: Comment Class
//      Author: Ziming Li
//     Created: 2019-10-19
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_COMMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_COMMENT_H_

#include "third_party/blink/renderer/core/dom/character_data.h"

namespace blink {

class CORE_EXPORT Comment final : public CharacterData {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static Comment* Create(Document&, const String&);

 private:
  Comment(Document&, const String&);

  String nodeName() const override;
  NodeType getNodeType() const override;
  Node* Clone(Document&, CloneChildrenFlag) const override;
#ifndef BLINKIT_CRAWLER_ONLY
  void DetachLayoutTree(const AttachContext&) final {}
#endif
};

DEFINE_NODE_TYPE_CASTS(Comment, getNodeType() == Node::kCommentNode);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_COMMENT_H_
