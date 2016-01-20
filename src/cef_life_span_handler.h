// Copyright (c) 2014 The cefcapi authors. All rights reserved.
// License: BSD 3-clause.
// Website: https://github.com/CzarekTomczak/cefcapi

#pragma once

#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_life_span_handler_capi.h"

typedef struct {
	cef_client_t client;
	cef_browser_t *browser;
	void (*on_load_end)();
} client_t;

typedef struct {
	cef_life_span_handler_t handler;
	client_t *client;
} life_span_handler_t;

static void ready();

///
// Called on the IO thread before a new popup browser is created. The
// |browser| and |frame| values represent the source of the popup request. The
// |target_url| and |target_frame_name| values indicate where the popup
// browser should navigate and may be NULL if not specified with the request.
// The |target_disposition| value indicates where the user intended to open
// the popup (e.g. current tab, new tab, etc). The |user_gesture| value will
// be true (1) if the popup was opened via explicit user gesture (e.g.
// clicking a link) or false (0) if the popup opened automatically (e.g. via
// the DomContentLoaded event). The |popupFeatures| structure contains
// additional information about the requested popup window. To allow creation
// of the popup browser optionally modify |windowInfo|, |client|, |settings|
// and |no_javascript_access| and return false (0). To cancel creation of the
// popup browser return true (1). The |client| and |settings| values will
// default to the source browser's values. If the |no_javascript_access| value
// is set to false (0) the new browser will not be scriptable and may not be
// hosted in the same renderer process as the source browser.
int CEF_CALLBACK on_before_popup(struct _cef_life_span_handler_t* self,
    struct _cef_browser_t* browser, struct _cef_frame_t* frame,
    const cef_string_t* target_url, const cef_string_t* target_frame_name,
    cef_window_open_disposition_t target_disposition, int user_gesture,
    const struct _cef_popup_features_t* popupFeatures,
    struct _cef_window_info_t* windowInfo, struct _cef_client_t** client,
    struct _cef_browser_settings_t* settings, int* no_javascript_access) {
    	return 0;
}

///
// Called after a new browser is created.
///
void CEF_CALLBACK on_after_created(struct _cef_life_span_handler_t* self,
    struct _cef_browser_t* browser) {
    	life_span_handler_t *handler = (life_span_handler_t *)self;
    	handler->client->browser = browser;
	ready();
}

///
// Called when a modal window is about to display and the modal loop should
// begin running. Return false (0) to use the default modal loop
// implementation or true (1) to use a custom implementation.
///
int CEF_CALLBACK run_modal(struct _cef_life_span_handler_t* self,
    struct _cef_browser_t* browser) {
    	return 0;
}

///
// Called when a browser has recieved a request to close. This may result
// directly from a call to cef_browser_host_t::close_browser() or indirectly
// if the browser is a top-level OS window created by CEF and the user
// attempts to close the window. This function will be called after the
// JavaScript 'onunload' event has been fired. It will not be called for
// browsers after the associated OS window has been destroyed (for those
// browsers it is no longer possible to cancel the close).
//
// If CEF created an OS window for the browser returning false (0) will send
// an OS close notification to the browser window's top-level owner (e.g.
// WM_CLOSE on Windows, performClose: on OS-X and "delete_event" on Linux). If
// no OS window exists (window rendering disabled) returning false (0) will
// cause the browser object to be destroyed immediately. Return true (1) if
// the browser is parented to another window and that other window needs to
// receive close notification via some non-standard technique.
//
// If an application provides its own top-level window it should handle OS
// close notifications by calling cef_browser_host_t::CloseBrowser(false (0))
// instead of immediately closing (see the example below). This gives CEF an
// opportunity to process the 'onbeforeunload' event and optionally cancel the
// close before do_close() is called.
//
// The cef_life_span_handler_t::on_before_close() function will be called
// immediately before the browser object is destroyed. The application should
// only exit after on_before_close() has been called for all existing
// browsers.
//
// If the browser represents a modal window and a custom modal loop
// implementation was provided in cef_life_span_handler_t::run_modal() this
// callback should be used to restore the opener window to a usable state.
//
// By way of example consider what should happen during window close when the
// browser is parented to an application-provided top-level OS window. 1.
// User clicks the window close button which sends an OS close
//     notification (e.g. WM_CLOSE on Windows, performClose: on OS-X and
//     "delete_event" on Linux).
// 2.  Application's top-level window receives the close notification and:
//     A. Calls CefBrowserHost::CloseBrowser(false).
//     B. Cancels the window close.
// 3.  JavaScript 'onbeforeunload' handler executes and shows the close
//     confirmation dialog (which can be overridden via
//     CefJSDialogHandler::OnBeforeUnloadDialog()).
// 4.  User approves the close. 5.  JavaScript 'onunload' handler executes. 6.
// Application's do_close() handler is called. Application will:
//     A. Set a flag to indicate that the next close attempt will be allowed.
//     B. Return false.
// 7.  CEF sends an OS close notification. 8.  Application's top-level window
// receives the OS close notification and
//     allows the window to close based on the flag from #6B.
// 9.  Browser OS window is destroyed. 10. Application's
// cef_life_span_handler_t::on_before_close() handler is called and
//     the browser object is destroyed.
// 11. Application exits by calling cef_quit_message_loop() if no other
// browsers
//     exist.
///
int CEF_CALLBACK do_close(struct _cef_life_span_handler_t* self,
    struct _cef_browser_t* browser) {
    	return 0;
}

///
// Called just before a browser is destroyed. Release all references to the
// browser object and do not attempt to execute any functions on the browser
// object after this callback returns. If this is a modal window and a custom
// modal loop implementation was provided in run_modal() this callback should
// be used to exit the custom modal loop. See do_close() documentation for
// additional usage information.
///
void CEF_CALLBACK on_before_close(struct _cef_life_span_handler_t* self,
    struct _cef_browser_t* browser)
{
	cef_quit_message_loop();
}
