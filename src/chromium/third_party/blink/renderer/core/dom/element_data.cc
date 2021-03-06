// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: element_data.cc
// Description: ElementData Class
//      Author: Ziming Li
//     Created: 2019-10-31
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
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

#include "third_party/blink/renderer/core/dom/element_data.h"

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"
#ifndef BLINKIT_CRAWLER_ONLY
#   include "third_party/blink/renderer/core/css/css_property_value_set.h"
#endif

namespace blink {

ElementData::ElementData()
    : is_unique_(true),
      array_size_(0),
      presentation_attribute_style_is_dirty_(false),
      style_attribute_is_dirty_(false),
      animated_svg_attributes_are_dirty_(false) {}

ElementData::ElementData(unsigned array_size)
    : is_unique_(false),
      array_size_(array_size),
      presentation_attribute_style_is_dirty_(false),
      style_attribute_is_dirty_(false),
      animated_svg_attributes_are_dirty_(false) {}

ElementData::ElementData(const ElementData& other, bool is_unique)
    : is_unique_(is_unique),
      array_size_(is_unique ? 0 : other.Attributes().size()),
      presentation_attribute_style_is_dirty_(
          other.presentation_attribute_style_is_dirty_),
      style_attribute_is_dirty_(other.style_attribute_is_dirty_),
      animated_svg_attributes_are_dirty_(
          other.animated_svg_attributes_are_dirty_),
      class_names_(other.class_names_),
      id_for_style_resolution_(other.id_for_style_resolution_) {
  // NOTE: The inline style is copied by the subclass copy constructor since we
  // don't know what to do with it here.
}

std::shared_ptr<UniqueElementData> ElementData::MakeUniqueCopy() const {
  if (IsUnique())
    return base::WrapShared(new UniqueElementData(ToUniqueElementData(*this)));
  return base::WrapShared(new UniqueElementData(ToShareableElementData(*this)));
}

bool ElementData::IsEquivalent(const ElementData* other) const {
  AttributeCollection attributes = Attributes();
  if (!other)
    return attributes.IsEmpty();

  AttributeCollection other_attributes = other->Attributes();
  if (attributes.size() != other_attributes.size())
    return false;

  for (const Attribute& attribute : attributes) {
    const Attribute* other_attr = other_attributes.Find(attribute.GetName());
    if (!other_attr || attribute.Value() != other_attr->Value())
      return false;
  }
  return true;
}

ShareableElementData::ShareableElementData(const Vector<Attribute>& attributes)
    : ElementData(attributes.size()) {
  attribute_array_ = reinterpret_cast<Attribute *>(malloc(sizeof(Attribute) * array_size_));
  for (unsigned i = 0; i < array_size_; ++i)
    new (&attribute_array_[i]) Attribute(attributes[i]);
}

ShareableElementData::~ShareableElementData() {
  for (unsigned i = 0; i < array_size_; ++i)
    attribute_array_[i].~Attribute();
  free(attribute_array_);
}

ShareableElementData::ShareableElementData(const UniqueElementData& other)
    : ElementData(other, false) {
  DCHECK(!other.presentation_attribute_style_);

#ifndef BLINKIT_CRAWLER_ONLY
  if (other.inline_style_) {
    inline_style_ = other.inline_style_->ImmutableCopyIfNeeded();
  }
#endif

  attribute_array_ = reinterpret_cast<Attribute *>(malloc(sizeof(Attribute) * array_size_));
  for (unsigned i = 0; i < array_size_; ++i)
    new (&attribute_array_[i]) Attribute(other.attribute_vector_.at(i));
}

std::shared_ptr<ShareableElementData> ShareableElementData::CreateWithAttributes(
    const Vector<Attribute>& attributes) {
  return base::WrapShared(new ShareableElementData(attributes));
}

UniqueElementData::UniqueElementData() = default;

UniqueElementData::UniqueElementData(const UniqueElementData& other)
    : ElementData(other, true),
      presentation_attribute_style_(other.presentation_attribute_style_),
      attribute_vector_(other.attribute_vector_) {
#ifndef BLINKIT_CRAWLER_ONLY
  inline_style_ =
      other.inline_style_ ? other.inline_style_->MutableCopy() : nullptr;
#endif
}

UniqueElementData::UniqueElementData(const ShareableElementData& other)
    : ElementData(other, true) {
#ifndef BLINKIT_CRAWLER_ONLY
  // An ShareableElementData should never have a mutable inline
  // CSSPropertyValueSet attached.
  DCHECK(!other.inline_style_ || !other.inline_style_->IsMutable());
  inline_style_ = other.inline_style_;
#endif

  unsigned length = other.Attributes().size();
  attribute_vector_.ReserveCapacity(length);
  for (unsigned i = 0; i < length; ++i)
    attribute_vector_.UncheckedAppend(other.attribute_array_[i]);
}

std::shared_ptr<UniqueElementData> UniqueElementData::Create() {
  return base::WrapShared(new UniqueElementData);
}

std::shared_ptr<ShareableElementData> UniqueElementData::MakeShareableCopy() const {
  return base::WrapShared(new ShareableElementData(*this));
}

}  // namespace blink
