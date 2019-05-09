// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: WebLocalFrame.h
// Description: WebLocalFrame Class
//      Author: Ziming Li
//     Created: 2019-05-09
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebLocalFrame_h
#define WebLocalFrame_h

#include "WebFrame.h"
#include "WebFrameLoadType.h"

namespace blink {

enum class WebAppBannerPromptReply;
enum class WebSandboxFlags;
enum class WebTreeScopeType;
class WebAutofillClient;
class WebContentSettingsClient;
class WebDevToolsAgent;
class WebDevToolsAgentClient;
class WebFrameClient;
class WebNode;
class WebScriptExecutionCallback;
class WebSuspendableTask;
class WebTestInterfaceFactory;
struct WebPrintPresetOptions;

// Interface for interacting with in process frames. This contains methods that
// require interacting with a frame's document.
// FIXME: Move lots of methods from WebFrame in here.
class WebLocalFrame : public WebFrame {
public:
    // Creates a WebFrame. Delete this WebFrame by calling WebFrame::close().
    // It is valid to pass a null client pointer.
    BLINK_EXPORT static WebLocalFrame* create(WebTreeScopeType, WebFrameClient*);

    // Used to create a provisional local frame in prepration for replacing a
    // remote frame if the load commits. The returned frame is only partially
    // attached to the frame tree: it has the same parent as its potential
    // replacee but is invisible to the rest of the frames in the frame tree.
    // If the load commits, call swap() to fully attach this frame.
    BLINK_EXPORT static WebLocalFrame* createProvisional(WebFrameClient*, WebRemoteFrame*, WebSandboxFlags, const WebFrameOwnerProperties&);

    // Initialization ---------------------------------------------------------

    virtual void setAutofillClient(WebAutofillClient*) = 0;
    virtual WebAutofillClient* autofillClient() = 0;

    // Basic properties ---------------------------------------------------

    // Updates the scrolling and margin properties in the frame's FrameOwner.
    // This is used when this frame's parent is in another process and it
    // dynamically updates these properties.
    // TODO(dcheng): Currently, the update only takes effect on next frame
    // navigation.  This matches the in-process frame behavior.
    virtual void setFrameOwnerProperties(const WebFrameOwnerProperties&) = 0;

    // Hierarchy ----------------------------------------------------------

    // Get the highest-level LocalFrame in this frame's in-process subtree.
    virtual WebLocalFrame* localRoot() = 0;

    // Navigation Ping --------------------------------------------------------
    virtual void sendPings(const WebNode& contextNode, const WebURL& destinationURL) = 0;

    // Navigation ----------------------------------------------------------

    // Returns a WebURLRequest corresponding to the reload of the current
    // HistoryItem.
    virtual WebURLRequest requestForReload(WebFrameLoadType,
        const WebURL& overrideURL = WebURL()) const = 0;

    // Load the given URL. For history navigations, a valid WebHistoryItem
    // should be given, as well as a WebHistoryLoadType.
    // TODO(clamy): Remove the reload, reloadWithOverrideURL, loadHistoryItem
    // loadRequest functions in WebFrame once RenderFrame only calls loadRequest.
    virtual void load(const WebURLRequest&, WebFrameLoadType = WebFrameLoadType::Standard,
        const WebHistoryItem& = WebHistoryItem(),
        WebHistoryLoadType = WebHistoryDifferentDocumentLoad) = 0;

    // Navigation State -------------------------------------------------------

    // Returns true if the current frame's load event has not completed.
    virtual bool isLoading() const = 0;

    // Returns true if any resource load is currently in progress. Exposed
    // primarily for use in layout tests. You probably want isLoading()
    // instead.
    virtual bool isResourceLoadInProgress() const = 0;

    // Returns true if there is a pending redirect or location change.
    // This could be caused by:
    // * an HTTP Refresh header
    // * an X-Frame-Options header
    // * the respective http-equiv meta tags
    // * window.location value being mutated
    // * CSP policy block
    // * reload
    // * form submission
    virtual bool isNavigationScheduled() const = 0;

    // Override the normal rules for whether a load has successfully committed
    // in this frame. Used to propagate state when this frame has navigated
    // cross process.
    virtual void setCommittedFirstRealLoad() = 0;


    // Scripting --------------------------------------------------------------
    // Executes script in the context of the current page and returns the value
    // that the script evaluated to with callback. Script execution can be
    // suspend.
    virtual void requestExecuteScriptAndReturnValue(const WebScriptSource&,
        bool userGesture, WebScriptExecutionCallback*) = 0;

    // worldID must be > 0 (as 0 represents the main world).
    // worldID must be < EmbedderWorldIdLimit, high number used internally.
    virtual void requestExecuteScriptInIsolatedWorld(
        int worldID, const WebScriptSource* sourceIn, unsigned numSources,
        int extensionGroup, bool userGesture, WebScriptExecutionCallback*) = 0;

    // Run the task when the context of the current page is not suspended
    // otherwise run it on context resumed.
    // Method takes ownership of the passed task.
    virtual void requestRunTask(WebSuspendableTask*) const = 0;

    // Associates an isolated world with human-readable name which is useful for
    // extension debugging.
    virtual void setIsolatedWorldHumanReadableName(int worldID, const WebString&) = 0;


    // Selection --------------------------------------------------------------

    // Moves the selection extent point. This function does not allow the
    // selection to collapse. If the new extent is set to the same position as
    // the current base, this function will do nothing.
    virtual void moveRangeSelectionExtent(const WebPoint&) = 0;
    // Replaces the selection with the input string.
    virtual void replaceSelection(const WebString&) = 0;

    // Spell-checking support -------------------------------------------------
    virtual void replaceMisspelledRange(const WebString&) = 0;

    // Content Settings -------------------------------------------------------

    virtual void setContentSettingsClient(WebContentSettingsClient*) = 0;

    // Image reload -----------------------------------------------------------

    // If the provided node is an image, reload the image disabling Lo-Fi.
    virtual void reloadImage(const WebNode&) = 0;

    // Feature usage logging --------------------------------------------------

    virtual void didCallAddSearchProvider() = 0;
    virtual void didCallIsSearchProviderInstalled() = 0;

protected:
    explicit WebLocalFrame(WebTreeScopeType scope) : WebFrame(scope) { }
};

} // namespace blink

#endif // WebLocalFrame_h
