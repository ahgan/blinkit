// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: DOMWindow.cpp
// Description: DOMWindow Class
//      Author: Ziming Li
//     Created: 2019-02-18
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/DOMWindow.h"

#include "core/dom/Document.h"
#include "core/dom/ExceptionCode.h"
#include "core/dom/ExecutionContext.h"
#include "core/dom/SecurityContext.h"
#include "core/frame/Frame.h"
#include "core/frame/FrameClient.h"
#include "core/frame/FrameConsole.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/Location.h"
#include "core/frame/Settings.h"
#include "core/frame/UseCounter.h"
#include "core/input/EventHandler.h"
#include "core/inspector/ConsoleMessageStorage.h"
#include "core/inspector/InspectorInstrumentation.h"
#include "core/loader/FrameLoaderClient.h"
#include "core/loader/MixedContentChecker.h"
#include "core/page/ChromeClient.h"
#include "core/page/FocusController.h"
#include "core/page/Page.h"
#include "platform/weborigin/KURL.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

DOMWindow::DOMWindow()
    : m_windowIsClosing(false)
{
}

DOMWindow::~DOMWindow()
{
}

const AtomicString& DOMWindow::interfaceName() const
{
    return EventTargetNames::DOMWindow;
}

Location* DOMWindow::location() const
{
    if (!m_location)
        m_location = Location::create(frame());
    return m_location.get();
}

bool DOMWindow::closed() const
{
    return m_windowIsClosing || !frame() || !frame()->host();
}

unsigned DOMWindow::length() const
{
    return frame() ? frame()->tree().scopedChildCount() : 0;
}

DOMWindow* DOMWindow::self() const
{
    if (!frame())
        return nullptr;

    return frame()->domWindow();
}

DOMWindow* DOMWindow::opener() const
{
    // FIXME: Use FrameTree to get opener as well, to simplify logic here.
    if (!frame() || !frame()->client())
        return nullptr;

    Frame* opener = frame()->client()->opener();
    return opener ? opener->domWindow() : nullptr;
}

DOMWindow* DOMWindow::parent() const
{
    if (!frame())
        return nullptr;

    Frame* parent = frame()->tree().parent();
    return parent ? parent->domWindow() : frame()->domWindow();
}

DOMWindow* DOMWindow::top() const
{
    if (!frame())
        return nullptr;

    return frame()->tree().top()->domWindow();
}

DOMWindow* DOMWindow::anonymousIndexedGetter(uint32_t index) const
{
    if (!frame())
        return nullptr;

    Frame* child = frame()->tree().scopedChild(index);
    return child ? child->domWindow() : nullptr;
}

bool DOMWindow::isCurrentlyDisplayedInFrame() const
{
    if (frame())
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(frame()->domWindow() == this);
    return frame() && frame()->host();
}

bool DOMWindow::isInsecureScriptAccess(LocalDOMWindow& callingWindow, const String& urlString)
{
    if (!protocolIsJavaScript(urlString))
        return false;

    // If this DOMWindow isn't currently active in the Frame, then there's no
    // way we should allow the access.
    if (isCurrentlyDisplayedInFrame()) {
        // FIXME: Is there some way to eliminate the need for a separate "callingWindow == this" check?
        if (&callingWindow == this)
            return false;

        // FIXME: The name canAccess seems to be a roundabout way to ask "can execute script".
        // Can we name the SecurityOrigin function better to make this more clear?
        if (callingWindow.document()->securityOrigin()->canAccessCheckSuborigins(frame()->securityContext()->securityOrigin()))
            return false;
    }

    callingWindow.printErrorMessage(crossDomainAccessErrorMessage(&callingWindow));
    return true;
}

void DOMWindow::resetLocation()
{
    // Location needs to be reset manually because it doesn't inherit from DOMWindowProperty.
    // DOMWindowProperty is local-only, and Location needs to support remote windows, too.
    if (m_location) {
        m_location->reset();
        m_location = nullptr;
    }
}

bool DOMWindow::isSecureContext() const
{
    if (!frame())
        return false;

    return document()->isSecureContext(ExecutionContext::StandardSecureContextCheck);
}

// FIXME: Once we're throwing exceptions for cross-origin access violations, we will always sanitize the target
// frame details, so we can safely combine 'crossDomainAccessErrorMessage' with this method after considering
// exactly which details may be exposed to JavaScript.
//
// http://crbug.com/17325
String DOMWindow::sanitizedCrossDomainAccessErrorMessage(const LocalDOMWindow* callingWindow) const
{
    if (!callingWindow || !callingWindow->document() || !frame())
        return String();

    const KURL& callingWindowURL = callingWindow->document()->url();
    if (callingWindowURL.isNull())
        return String();

    ASSERT(!callingWindow->document()->securityOrigin()->canAccessCheckSuborigins(frame()->securityContext()->securityOrigin()));

    const SecurityOrigin* activeOrigin = callingWindow->document()->securityOrigin();
    String message = "Blocked a frame with origin \"" + activeOrigin->toString() + "\" from accessing a cross-origin frame.";

    // FIXME: Evaluate which details from 'crossDomainAccessErrorMessage' may safely be reported to JavaScript.

    return message;
}

String DOMWindow::crossDomainAccessErrorMessage(const LocalDOMWindow* callingWindow) const
{
    if (!callingWindow || !callingWindow->document() || !frame())
        return String();

    const KURL& callingWindowURL = callingWindow->document()->url();
    if (callingWindowURL.isNull())
        return String();

    // FIXME: This message, and other console messages, have extra newlines. Should remove them.
    const SecurityOrigin* activeOrigin = callingWindow->document()->securityOrigin();
    const SecurityOrigin* targetOrigin = frame()->securityContext()->securityOrigin();
    ASSERT(!activeOrigin->canAccessCheckSuborigins(targetOrigin));

    String message = "Blocked a frame with origin \"" + activeOrigin->toString() + "\" from accessing a frame with origin \"" + targetOrigin->toString() + "\". ";

    // Sandbox errors: Use the origin of the frames' location, rather than their actual origin (since we know that at least one will be "null").
    KURL activeURL = callingWindow->document()->url();
    // TODO(alexmos): RemoteFrames do not have a document, and their URLs
    // aren't replicated.  For now, construct the URL using the replicated
    // origin for RemoteFrames. If the target frame is remote and sandboxed,
    // there isn't anything else to show other than "null" for its origin.
    KURL targetURL = isLocalDOMWindow() ? document()->url() : KURL(KURL(), targetOrigin->toString());

    // Protocol errors: Use the URL's protocol rather than the origin's protocol so that we get a useful message for non-heirarchal URLs like 'data:'.
    if (targetOrigin->protocol() != activeOrigin->protocol())
        return message + " The frame requesting access has a protocol of \"" + activeURL.protocol() + "\", the frame being accessed has a protocol of \"" + targetURL.protocol() + "\". Protocols must match.\n";

    // 'document.domain' errors.
    if (targetOrigin->domainWasSetInDOM() && activeOrigin->domainWasSetInDOM())
        return message + "The frame requesting access set \"document.domain\" to \"" + activeOrigin->domain() + "\", the frame being accessed set it to \"" + targetOrigin->domain() + "\". Both must set \"document.domain\" to the same value to allow access.";
    if (activeOrigin->domainWasSetInDOM())
        return message + "The frame requesting access set \"document.domain\" to \"" + activeOrigin->domain() + "\", but the frame being accessed did not. Both must set \"document.domain\" to the same value to allow access.";
    if (targetOrigin->domainWasSetInDOM())
        return message + "The frame being accessed set \"document.domain\" to \"" + targetOrigin->domain() + "\", but the frame requesting access did not. Both must set \"document.domain\" to the same value to allow access.";

    // Default.
    return message + "Protocols, domains, and ports must match.";
}

void DOMWindow::close(ExecutionContext* context)
{
    if (!frame() || !frame()->isMainFrame())
        return;

    Page* page = frame()->page();
    if (!page)
        return;

    Document* activeDocument = nullptr;
    if (context) {
        ASSERT(isMainThread());
        activeDocument = toDocument(context);
        if (!activeDocument)
            return;

        if (!activeDocument->frame() || !activeDocument->frame()->canNavigate(*frame()))
            return;
    }

    Settings* settings = frame()->settings();
    bool allowScriptsToCloseWindows = settings && settings->allowScriptsToCloseWindows();

    if (!page->openedByDOM() && frame()->client()->backForwardLength() > 1 && !allowScriptsToCloseWindows) {
        if (activeDocument) {
            activeDocument->domWindow()->frameConsole()->addMessage(ConsoleMessage::create(JSMessageSource, WarningMessageLevel, "Scripts may close only the windows that were opened by it."));
        }
        return;
    }

    if (!frame()->shouldClose())
        return;

    InspectorInstrumentation::willCloseWindow(context);

    page->chromeClient().closeWindowSoon();

    // So as to make window.closed return the expected result
    // after window.close(), separately record the to-be-closed
    // state of this window. Scripts may access window.closed
    // before the deferred close operation has gone ahead.
    m_windowIsClosing = true;
}

void DOMWindow::focus(ExecutionContext* context)
{
    if (!frame())
        return;

    Page* page = frame()->page();
    if (!page)
        return;

    ASSERT(context);

    bool allowFocus = context->isWindowInteractionAllowed();
    if (allowFocus) {
        context->consumeWindowInteraction();
    } else {
        ASSERT(isMainThread());
        allowFocus = opener() && (opener() != this) && (toDocument(context)->domWindow() == opener());
    }

    // If we're a top level window, bring the window to the front.
    if (frame()->isMainFrame() && allowFocus)
        page->chromeClient().focus();

    page->focusController().focusDocumentView(frame(), true /* notifyEmbedder */);
}

DEFINE_TRACE(DOMWindow)
{
    visitor->trace(m_location);
    EventTargetWithInlineData::trace(visitor);
}

} // namespace blink
