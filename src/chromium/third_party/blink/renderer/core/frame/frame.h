// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: frame.h
// Description: Frame Class
//      Author: Ziming Li
//     Created: 2019-09-12
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999-2001 Lars Knoll <knoll@kde.org>
 *                     1999-2001 Antti Koivisto <koivisto@kde.org>
 *                     2000-2001 Simon Hausmann <hausmann@kde.org>
 *                     2000-2001 Dirk Mueller <mueller@kde.org>
 *                     2000 Stefan Schimanski <1Stein@gmx.de>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
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

#ifndef BLINKIT_BLINK_FRAME_H
#define BLINKIT_BLINK_FRAME_H

#pragma once

#include "third_party/blink/renderer/core/frame/frame_lifecycle.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class DOMWindow;
class FrameClient;
class Page;

class Frame : public GarbageCollectedFinalized<Frame>
{
public:
    virtual ~Frame(void);

    virtual bool IsLocalFrame(void) const = 0;

    FrameClient* Client(void) const { return &m_client; }
    DOMWindow* DomWindow(void) const { return m_domWindow.get(); }

    bool IsAttached(void) const { return m_lifecycle.GetState() == FrameLifecycle::kAttached; }

    // IsLoading() is true when the embedder should think a load is in progress.
    // In the case of LocalFrames, it means that the frame has sent a
    // didStartLoading() callback, but not the matching didStopLoading(). Inside
    // blink, you probably want Document::loadEventFinished() instead.
    bool IsLoading(void) const { return m_isLoading; }
    void SetIsLoading(bool isLoading) { m_isLoading = isLoading; }
protected:
    Frame(FrameClient &client, Page *page);

    FrameClient &m_client;
    Member<Page> m_page;
    std::unique_ptr<DOMWindow> m_domWindow;
private:
    FrameLifecycle m_lifecycle;
    bool m_isLoading = false;
};

} // namespace blink

#endif // BLINKIT_BLINK_FRAME_H
