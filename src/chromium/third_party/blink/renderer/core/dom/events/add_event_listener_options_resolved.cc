// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: add_event_listener_options_resolved.cc
// Description: AddEventListenerOptionsResolved Class
//      Author: Ziming Li
//     Created: 2019-12-21
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/events/add_event_listener_options_resolved.h"

namespace blink {

AddEventListenerOptionsResolved::AddEventListenerOptionsResolved()
    : passive_forced_for_document_target_(false), passive_specified_(false) {}

AddEventListenerOptionsResolved::AddEventListenerOptionsResolved(
    const AddEventListenerOptions& options)
    : AddEventListenerOptions(options),
      passive_forced_for_document_target_(false),
      passive_specified_(false) {}

AddEventListenerOptionsResolved::~AddEventListenerOptionsResolved() = default;

}  // namespace blink
