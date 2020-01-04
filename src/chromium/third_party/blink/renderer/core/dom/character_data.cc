// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: character_data.cc
// Description: CharacterData Class
//      Author: Ziming Li
//     Created: 2019-10-30
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2013 Apple Inc. All
 * rights reserved.
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
 */

#include "third_party/blink/renderer/core/dom/character_data.h"

#include "base/numerics/checked_math.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/mutation_observer_interest_group.h"
#include "third_party/blink/renderer/core/dom/mutation_record.h"
// BKTODO: #include "third_party/blink/renderer/core/dom/processing_instruction.h"
#include "third_party/blink/renderer/core/dom/text.h"
// BKTODO: #include "third_party/blink/renderer/core/editing/frame_selection.h"
// BKTODO: #include "third_party/blink/renderer/core/events/mutation_event.h"
// BKTODO: #include "third_party/blink/renderer/core/probe/core_probes.h"
// BKTODO: #include "third_party/blink/renderer/platform/bindings/exception_state.h"

namespace blink {

void CharacterData::Atomize() {
  data_ = AtomicString(data_);
}

void CharacterData::setData(const String& data) {
  unsigned old_length = length();

  SetDataAndUpdate(data, 0, old_length, data.length(), kUpdateFromNonParser);
  ASSERT(false); // BKTODO:
#if 0
  GetDocument().DidRemoveText(*this, 0, old_length);
#endif
}

String CharacterData::substringData(unsigned offset,
                                    unsigned count,
                                    ExceptionState& exception_state) {
  ASSERT(false); // BKTODO:
#if 0
  if (offset > length()) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kIndexSizeError,
        "The offset " + String::Number(offset) +
            " is greater than the node's length (" + String::Number(length()) +
            ").");
    return String();
  }
#endif

  return data_.Substring(offset, count);
}

void CharacterData::ParserAppendData(const String& data) {
  String new_str = data_ + data;

  SetDataAndUpdate(new_str, data_.length(), 0, data.length(),
                   kUpdateFromParser);
}

void CharacterData::appendData(const String& data) {
  String new_str = data_ + data;

  SetDataAndUpdate(new_str, data_.length(), 0, data.length(),
                   kUpdateFromNonParser);

  // FIXME: Should we call textInserted here?
}

void CharacterData::insertData(unsigned offset,
                               const String& data,
                               ExceptionState& exception_state) {
  ASSERT(false); // BKTODO:
#if 0
  if (offset > length()) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kIndexSizeError,
        "The offset " + String::Number(offset) +
            " is greater than the node's length (" + String::Number(length()) +
            ").");
    return;
  }

  String new_str = data_;
  new_str.insert(data, offset);

  SetDataAndUpdate(new_str, offset, 0, data.length(), kUpdateFromNonParser);

  GetDocument().DidInsertText(*this, offset, data.length());
#endif
}

static bool ValidateOffsetCount(unsigned offset,
                                unsigned count,
                                unsigned length,
                                unsigned& real_count,
                                ExceptionState& exception_state) {
  ASSERT(false); // BKTODO:
#if 0
  if (offset > length) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kIndexSizeError,
        "The offset " + String::Number(offset) +
            " is greater than the node's length (" + String::Number(length) +
            ").");
    return false;
  }

  base::CheckedNumeric<unsigned> offset_count = offset;
  offset_count += count;

  if (!offset_count.IsValid() || offset + count > length)
    real_count = length - offset;
  else
    real_count = count;
#endif

  return true;
}

void CharacterData::deleteData(unsigned offset,
                               unsigned count,
                               ExceptionState& exception_state) {
  ASSERT(false); // BKTODO:
#if 0
  unsigned real_count = 0;
  if (!ValidateOffsetCount(offset, count, length(), real_count,
                           exception_state))
    return;

  String new_str = data_;
  new_str.Remove(offset, real_count);

  SetDataAndUpdate(new_str, offset, real_count, 0, kUpdateFromNonParser);

  GetDocument().DidRemoveText(*this, offset, real_count);
#endif
}

void CharacterData::replaceData(unsigned offset,
                                unsigned count,
                                const String& data,
                                ExceptionState& exception_state) {
  ASSERT(false); // BKTODO:
#if 0
  unsigned real_count = 0;
  if (!ValidateOffsetCount(offset, count, length(), real_count,
                           exception_state))
    return;

  String new_str = data_;
  new_str.Remove(offset, real_count);
  new_str.insert(data, offset);

  SetDataAndUpdate(new_str, offset, real_count, data.length(),
                   kUpdateFromNonParser);

  // update DOM ranges
  GetDocument().DidRemoveText(*this, offset, real_count);
  GetDocument().DidInsertText(*this, offset, data.length());
#endif
}

String CharacterData::nodeValue() const {
  return data_;
}

bool CharacterData::ContainsOnlyWhitespace() const {
  return data_.ContainsOnlyWhitespace();
}

void CharacterData::setNodeValue(const String& node_value) {
  setData(!node_value.IsNull() ? node_value : g_empty_string);
}

void CharacterData::SetDataAndUpdate(const String& new_data,
                                     unsigned offset_of_replaced_data,
                                     unsigned old_length,
                                     unsigned new_length,
                                     UpdateSource source) {
  String old_data = data_;
  data_ = new_data;

#ifdef BLINKIT_CRAWLER_ONLY
  ASSERT(IsTextNode());
#else
  DCHECK(!GetLayoutObject() || IsTextNode());
  if (IsTextNode())
    ToText(this)->UpdateTextLayoutObject(offset_of_replaced_data, old_length);
#endif

  if (source != kUpdateFromParser)
  {
    ASSERT(false); // BKTODO:
#if 0
    if (getNodeType() == kProcessingInstructionNode)
      ToProcessingInstruction(this)->DidAttributeChanged();

    GetDocument().NotifyUpdateCharacterData(this, offset_of_replaced_data,
                                            old_length, new_length);
#endif
  }

  GetDocument().IncDOMTreeVersion();
  DidModifyData(old_data, source);
}

void CharacterData::DidModifyData(const String& old_data, UpdateSource source) {
  if (MutationObserverInterestGroup* mutation_recipients =
          MutationObserverInterestGroup::CreateForCharacterDataMutation(*this))
    mutation_recipients->EnqueueMutationRecord(
        MutationRecord::CreateCharacterData(this, old_data));

  if (parentNode()) {
    ContainerNode::ChildrenChange change = {
        ContainerNode::kTextChanged, this, previousSibling(), nextSibling(),
        ContainerNode::kChildrenChangeSourceAPI};
    parentNode()->ChildrenChanged(change);
  }

  // Skip DOM mutation events if the modification is from parser.
  // Note that mutation observer events will still fire.
  // Spec: https://html.spec.whatwg.org/multipage/syntax.html#insert-a-character
  if (source != kUpdateFromParser && !IsInShadowTree()) {
    ASSERT(false); // BKTODO:
#if 0
    if (GetDocument().HasListenerType(
            Document::kDOMCharacterDataModifiedListener)) {
      DispatchScopedEvent(*MutationEvent::Create(
          EventTypeNames::DOMCharacterDataModified, Event::Bubbles::kYes,
          nullptr, old_data, data_));
    }
    DispatchSubtreeModifiedEvent();
#endif
  }
}

}  // namespace blink
