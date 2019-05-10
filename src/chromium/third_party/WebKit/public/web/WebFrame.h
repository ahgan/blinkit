// -------------------------------------------------
// BlinKit - blink Library
// -------------------------------------------------
//   File Name: WebFrame.h
// Description: WebFrame Class
//      Author: Ziming Li
//     Created: 2019-03-05
// -------------------------------------------------
// Copyright (C) 2019 MingYang Software Technology.
// -------------------------------------------------

/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef WebFrame_h
#define WebFrame_h

#include "WebCompositionUnderline.h"
#include "WebHistoryItem.h"
#include "WebIconURL.h"
#include "WebNode.h"
#include "WebURLLoaderOptions.h"
#include "public/platform/WebCanvas.h"
#include "public/platform/WebPrivateOwnPtr.h"
#include "public/platform/WebReferrerPolicy.h"
#include "public/platform/WebURL.h"
#include "public/platform/WebURLRequest.h"
#include "public/web/WebTreeScopeType.h"

struct NPObject;

namespace v8 {
class Context;
class Function;
class Object;
class Value;
template <class T> class Local;
}

namespace blink {

class Frame;
class OpenedFrameTracker;
class Visitor;
class WebData;
class WebDataSource;
class WebDocument;
class WebElement;
class WebFrameImplBase;
class WebLayer;
class WebLocalFrame;
class WebRange;
class WebRemoteFrame;
class WebSecurityOrigin;
class WebSharedWorkerRepositoryClient;
class WebString;
class WebURL;
class WebURLLoader;
class WebURLRequest;
class WebView;
enum class WebSandboxFlags;
struct WebConsoleMessage;
struct WebFindOptions;
struct WebFloatPoint;
struct WebFloatRect;
struct WebFrameOwnerProperties;
struct WebPoint;
struct WebPrintParams;
struct WebRect;
struct WebScriptSource;
struct WebSize;
struct WebURLLoaderOptions;

template <typename T> class WebVector;

// Frames may be rendered in process ('local') or out of process ('remote').
// A remote frame is always cross-site; a local frame may be either same-site or
// cross-site.
// WebFrame is the base class for both WebLocalFrame and WebRemoteFrame and
// contains methods that are valid on both local and remote frames, such as
// getting a frame's parent or its opener.
class WebFrame {
public:
    // Control of layoutTreeAsText output
    enum LayoutAsTextControl {
        LayoutAsTextNormal = 0,
        LayoutAsTextDebug = 1 << 0,
        LayoutAsTextPrinting = 1 << 1,
        LayoutAsTextWithLineTrees = 1 << 2
    };
    typedef unsigned LayoutAsTextControls;

    // FIXME: We already have blink::TextGranularity. For now we support only
    // a part of blink::TextGranularity.
    // Ideally it seems blink::TextGranularity should be broken up into
    // blink::TextGranularity and perhaps blink::TextBoundary and then
    // TextGranularity enum could be moved somewhere to public/, and we could
    // just use it here directly without introducing a new enum.
    enum TextGranularity {
        CharacterGranularity = 0,
        WordGranularity,
        TextGranularityLast = WordGranularity,
    };

    // Returns the number of live WebFrame objects, used for leak checking.
    BLINK_EXPORT static int instanceCount();

    virtual bool isWebLocalFrame() const = 0;
    virtual WebLocalFrame* toWebLocalFrame() = 0;

    // This method closes and deletes the WebFrame. This is typically called by
    // the embedder in response to a frame detached callback to the WebFrame
    // client.
    virtual void close() = 0;

    // Called by the embedder when it needs to detach the subtree rooted at this
    // frame.
    BLINK_EXPORT void detach();

    // Basic properties ---------------------------------------------------

    // The urls of the given combination types of favicon (if any) specified by
    // the document loaded in this frame. The iconTypesMask is a bit-mask of
    // WebIconURL::Type values, used to select from the available set of icon
    // URLs
    virtual WebVector<WebIconURL> iconURLs(int iconTypesMask) const = 0;

    // For a WebFrame with contents being rendered in another process, this
    // sets a layer for use by the in-process compositor. WebLayer should be
    // null if the content is being rendered in the current process.
    virtual void setRemoteWebLayer(WebLayer*) = 0;

    // Returns true if the frame is enforcing strict mixed content checking.
    BLINK_EXPORT bool shouldEnforceStrictMixedContentChecking() const;

    // Geometry -----------------------------------------------------------

    // NOTE: These routines do not force page layout so their results may
    // not be accurate if the page layout is out-of-date.

    // If set to false, do not draw scrollbars on this frame's view.
    virtual void setCanHaveScrollbars(bool) = 0;

    // The scroll offset from the top-left corner of the frame in pixels.
    virtual WebSize scrollOffset() const = 0;
    virtual void setScrollOffset(const WebSize&) = 0;

    // The size of the contents area.
    virtual WebSize contentsSize() const = 0;

    // Returns true if the contents (minus scrollbars) has non-zero area.
    virtual bool hasVisibleContent() const = 0;

    // Returns the visible content rect (minus scrollbars, in absolute coordinate)
    virtual WebRect visibleContentRect() const = 0;

    virtual bool hasHorizontalScrollbar() const = 0;
    virtual bool hasVerticalScrollbar() const = 0;

    // Hierarchy ----------------------------------------------------------

    // Returns the containing view.
    virtual WebView* view() const = 0;

    // Returns the frame that opened this frame or 0 if there is none.
    BLINK_EXPORT WebFrame* opener() const;

    // Sets the frame that opened this one or 0 if there is none.
    virtual void setOpener(WebFrame*);

    // Reset the frame that opened this frame to 0.
    // This is executed between layout tests runs
    void clearOpener() { setOpener(0); }

    // Content ------------------------------------------------------------

    virtual WebDocument document() const = 0;


    // Closing -------------------------------------------------------------

    // Runs beforeunload handlers for this frame, returning false if a
    // handler suppressed unloading.
    virtual bool dispatchBeforeUnloadEvent() = 0;

    // Runs unload handlers for this frame.
    virtual void dispatchUnloadEvent() = 0;


    // Scripting ----------------------------------------------------------

    // Executes script in the context of the current page.
    virtual void executeScript(const WebScriptSource&) = 0;

    // Executes JavaScript in a new world associated with the web frame.
    // The script gets its own global scope and its own prototypes for
    // intrinsic JavaScript objects (String, Array, and so-on). It also
    // gets its own wrappers for all DOM nodes and DOM constructors.
    // extensionGroup is an embedder-provided specifier that controls which
    // v8 extensions are loaded into the new context - see
    // blink::registerExtension for the corresponding specifier.
    //
    // worldID must be > 0 (as 0 represents the main world).
    // worldID must be < EmbedderWorldIdLimit, high number used internally.
    virtual void executeScriptInIsolatedWorld(
        int worldID, const WebScriptSource* sources, unsigned numSources,
        int extensionGroup) = 0;

    // Associates an isolated world (see above for description) with a security
    // origin. XMLHttpRequest instances used in that world will be considered
    // to come from that origin, not the frame's.
    virtual void setIsolatedWorldSecurityOrigin(
        int worldID, const WebSecurityOrigin&) = 0;

    // Associates a content security policy with an isolated world. This policy
    // should be used when evaluating script in the isolated world, and should
    // also replace a protected resource's CSP when evaluating resources
    // injected into the DOM.
    //
    // FIXME: Setting this simply bypasses the protected resource's CSP. It
    //     doesn't yet restrict the isolated world to the provided policy.
    virtual void setIsolatedWorldContentSecurityPolicy(
        int worldID, const WebString&) = 0;

    // Logs to the console associated with this frame.
    virtual void addMessageToConsole(const WebConsoleMessage&) = 0;

    // Calls window.gc() if it is defined.
    virtual void collectGarbage() = 0;

    // Check if the scripting URL represents a mixed content condition relative
    // to this frame.
    virtual bool checkIfRunInsecureContent(const WebURL&) const = 0;

    // Returns true if the WebFrame currently executing JavaScript has access
    // to the given WebFrame, or false otherwise.
    BLINK_EXPORT static bool scriptCanAccess(WebFrame*);


    // Navigation ----------------------------------------------------------

    // Reload the current document.
    // True |ignoreCache| explicitly bypasses caches.
    // False |ignoreCache| revalidates any existing cache entries.
    virtual void reload(bool ignoreCache = false) = 0;

    // This is used for situations where we want to reload a different URL because of a redirect.
    virtual void reloadWithOverrideURL(const WebURL& overrideUrl, bool ignoreCache = false) = 0;

    // Load the given URL.
    virtual void loadRequest(const WebURLRequest&) = 0;

    // Loads the given data with specific mime type and optional text
    // encoding.  For HTML data, baseURL indicates the security origin of
    // the document and is used to resolve links.  If specified,
    // unreachableURL is reported via WebDataSource::unreachableURL.  If
    // replace is false, then this data will be loaded as a normal
    // navigation.  Otherwise, the current history item will be replaced.
    virtual void loadData(const WebData& data,
                          const WebString& mimeType,
                          const WebString& textEncoding,
                          const WebURL& baseURL,
                          const WebURL& unreachableURL = WebURL(),
                          bool replace = false) = 0;

    // This method is short-hand for calling LoadData, where mime_type is
    // "text/html" and text_encoding is "UTF-8".
    virtual void loadHTMLString(const WebData& html,
                                const WebURL& baseURL,
                                const WebURL& unreachableURL = WebURL(),
                                bool replace = false) = 0;

    // Stops any pending loads on the frame and its children.
    virtual void stopLoading() = 0;

    // Returns the data source that is currently loading.  May be null.
    virtual WebDataSource* provisionalDataSource() const = 0;

    // Returns the data source that is currently loaded.
    virtual WebDataSource* dataSource() const = 0;

    // Sets the referrer for the given request to be the specified URL or
    // if that is null, then it sets the referrer to the referrer that the
    // frame would use for subresources.  NOTE: This method also filters
    // out invalid referrers (e.g., it is invalid to send a HTTPS URL as
    // the referrer for a HTTP request).
    virtual void setReferrerForRequest(WebURLRequest&, const WebURL&) = 0;

    // Called to associate the WebURLRequest with this frame.  The request
    // will be modified to inherit parameters that allow it to be loaded.
    // This method ends up triggering WebFrameClient::willSendRequest.
    // DEPRECATED: Please use createAssociatedURLLoader instead.
    virtual void dispatchWillSendRequest(WebURLRequest&) = 0;

    // Returns a WebURLLoader that is associated with this frame.  The loader
    // will, for example, be cancelled when WebFrame::stopLoading is called.
    // FIXME: stopLoading does not yet cancel an associated loader!!
    virtual WebURLLoader* createAssociatedURLLoader(const WebURLLoaderOptions& = WebURLLoaderOptions()) = 0;

    // Returns the number of registered unload listeners.
    virtual unsigned unloadListenerCount() const = 0;

    // Will return true if between didStartLoading and didStopLoading notifications.
    virtual bool isLoading() const;


    // Editing -------------------------------------------------------------

    virtual void insertText(const WebString& text) = 0;

    virtual void setMarkedText(const WebString& text, unsigned location, unsigned length) = 0;
    virtual void unmarkText() = 0;
    virtual bool hasMarkedText() const = 0;

    virtual WebRange markedRange() const = 0;

    // Returns the text range rectangle in the viepwort coordinate space.
    virtual bool firstRectForCharacterRange(unsigned location, unsigned length, WebRect&) const = 0;

    // Returns the index of a character in the Frame's text stream at the given
    // point. The point is in the viewport coordinate space. Will return
    // WTF::notFound if the point is invalid.
    virtual size_t characterIndexForPoint(const WebPoint&) const = 0;

    // Supports commands like Undo, Redo, Cut, Copy, Paste, SelectAll,
    // Unselect, etc. See EditorCommand.cpp for the full list of supported
    // commands.
    virtual bool executeCommand(const WebString&, const WebNode& = WebNode()) = 0;
    virtual bool executeCommand(const WebString&, const WebString& value, const WebNode& = WebNode()) = 0;
    virtual bool isCommandEnabled(const WebString&) const = 0;

    // Selection -----------------------------------------------------------

    virtual bool hasSelection() const = 0;

    virtual WebRange selectionRange() const = 0;

    virtual WebString selectionAsText() const = 0;
    virtual WebString selectionAsMarkup() const = 0;

    // Expands the selection to a word around the caret and returns
    // true. Does nothing and returns false if there is no caret or
    // there is ranged selection.
    virtual bool selectWordAroundCaret() = 0;

    // DEPRECATED: Use moveRangeSelection.
    virtual void selectRange(const WebPoint& base, const WebPoint& extent) = 0;

    virtual void selectRange(const WebRange&) = 0;

    // Move the current selection to the provided viewport point/points. If the
    // current selection is editable, the new selection will be restricted to
    // the root editable element.
    // |TextGranularity| represents character wrapping granularity. If
    // WordGranularity is set, WebFrame extends selection to wrap word.
    virtual void moveRangeSelection(const WebPoint& base, const WebPoint& extent, WebFrame::TextGranularity = CharacterGranularity) = 0;
    virtual void moveCaretSelection(const WebPoint&) = 0;

    virtual bool setEditableSelectionOffsets(int start, int end) = 0;
    virtual bool setCompositionFromExistingText(int compositionStart, int compositionEnd, const WebVector<WebCompositionUnderline>& underlines) = 0;
    virtual void extendSelectionAndDelete(int before, int after) = 0;

    virtual void setCaretVisible(bool) = 0;

    // CSS3 Paged Media ----------------------------------------------------

    // Returns true if page box (margin boxes and page borders) is visible.
    virtual bool isPageBoxVisible(int pageIndex) = 0;

    // Returns true if the page style has custom size information.
    virtual bool hasCustomPageSizeStyle(int pageIndex) = 0;

    // Returns the preferred page size and margins in pixels, assuming 96
    // pixels per inch. pageSize, marginTop, marginRight, marginBottom,
    // marginLeft must be initialized to the default values that are used if
    // auto is specified.
    virtual void pageSizeAndMarginsInPixels(int pageIndex,
                                            WebSize& pageSize,
                                            int& marginTop,
                                            int& marginRight,
                                            int& marginBottom,
                                            int& marginLeft) = 0;

    // Returns the value for a page property that is only defined when printing.
    // printBegin must have been called before this method.
    virtual WebString pageProperty(const WebString& propertyName, int pageIndex) = 0;

    // Find-in-page --------------------------------------------------------

    // Set the tickmarks for the frame. This will override the default tickmarks
    // generated by find results. If this is called with an empty array, the
    // default behavior will be restored.
    virtual void setTickmarks(const WebVector<WebRect>&) = 0;

    // Events --------------------------------------------------------------

    // Dispatches a message event on the current DOMWindow in this WebFrame.
    virtual void dispatchMessageEventWithOriginCheck(
        const WebSecurityOrigin& intendedTargetOrigin,
        const WebDOMEvent&) = 0;


    // Utility -------------------------------------------------------------

    // Returns the contents of this frame as a string.  If the text is
    // longer than maxChars, it will be clipped to that length.  WARNING:
    // This function may be slow depending on the number of characters
    // retrieved and page complexity.  For a typically sized page, expect
    // it to take on the order of milliseconds.
    //
    // If there is room, subframe text will be recursively appended. Each
    // frame will be separated by an empty line.
    virtual WebString contentAsText(size_t maxChars) const = 0;

    // Returns HTML text for the contents of this frame.  This is generated
    // from the DOM.
    virtual WebString contentAsMarkup() const = 0;

    // Returns a text representation of the render tree.  This method is used
    // to support layout tests.
    virtual WebString layoutTreeAsText(LayoutAsTextControls toShow = LayoutAsTextNormal) const = 0;

    // Calls markerTextForListItem() defined in core/layout/LayoutTreeAsText.h.
    virtual WebString markerTextForListItem(const WebElement&) const = 0;

    // Returns the bounds rect for current selection. If selection is performed
    // on transformed text, the rect will still bound the selection but will
    // not be transformed itself. If no selection is present, the rect will be
    // empty ((0,0), (0,0)).
    virtual WebRect selectionBoundsRect() const = 0;

    // Dumps the layer tree, used by the accelerated compositor, in
    // text form. This is used only by layout tests.
    virtual WebString layerTreeAsText(bool showDebugInfo = false) const = 0;

    virtual WebFrameImplBase* toImplBase() = 0;
    // TODO(dcheng): Fix const-correctness issues and remove this overload.
    virtual const WebFrameImplBase* toImplBase() const
    {
        return const_cast<WebFrame*>(this)->toImplBase();
    }

#if BLINK_IMPLEMENTATION
    static WebFrame* fromFrame(Frame*);

    bool inShadowTree() const { return m_scope == WebTreeScopeType::Shadow; }

#if ENABLE(OILPAN)
    static void traceFrames(Visitor*, WebFrame*);
    static void traceFrames(InlinedGlobalMarkingVisitor, WebFrame*);
    void clearWeakFrames(Visitor*);
    void clearWeakFrames(InlinedGlobalMarkingVisitor);
#endif
#endif

protected:
    explicit WebFrame(WebTreeScopeType);
    virtual ~WebFrame();

private:
#if BLINK_IMPLEMENTATION
#if ENABLE(OILPAN)
    friend class OpenedFrameTracker;

    static void traceFrame(Visitor*, WebFrame*);
    static void traceFrame(InlinedGlobalMarkingVisitor, WebFrame*);
    static bool isFrameAlive(const WebFrame*);

    template <typename VisitorDispatcher>
    static void traceFramesImpl(VisitorDispatcher, WebFrame*);
    template <typename VisitorDispatcher>
    void clearWeakFramesImpl(VisitorDispatcher);
    template <typename VisitorDispatcher>
    static void traceFrameImpl(VisitorDispatcher, WebFrame*);
#endif
#endif

    const WebTreeScopeType m_scope;

    WebFrame* m_opener;
    WebPrivateOwnPtr<OpenedFrameTracker> m_openedFrameTracker;
};

} // namespace blink

#endif
