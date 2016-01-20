// Copyright (c) 2014 The cefcapi authors. All rights reserved.
// License: BSD 3-clause.
// Website: https://github.com/CzarekTomczak/cefcapi

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#ifndef WINDOWLESS

#include <gdk/gdkx.h>

#include "gtk.h"
#endif
#include "cef_app.h"
#include "cef_client.h"
#include "cef_life_span_handler.h"
#include "cef_load_handler.h"
#include "cef_render_handler.h"

typedef struct {
	int argumentsExpected;
	char *commandName;
	char *argument;
} ReceivedCommand;

void CEF_CALLBACK get_frame_source(struct _cef_string_visitor_t* self,
    const cef_string_t* string) {
	printf("ok\n");
	cef_string_utf8_t out = {};
	cef_string_utf16_to_utf8(string->str, string->length, &out);
	printf("%zu\n", out.length);
	printf("%s\n", out.str);
	fflush(stdout);
};

void
handle_load_event()
{
	printf("ok\n");
	printf("0\n");
	fflush(stdout);
}

void processArgument(ReceivedCommand *cmd, const char *data, client_t *client,
    int *expectingDataSize) {
	if (cmd->argumentsExpected == -1) {
		int i = atoi(data);
		cmd->argumentsExpected = i;
	} else if (*expectingDataSize == -1) {
		int i = atoi(data);
		*expectingDataSize = i;
	} else {
		int len = strlen(data) + 1;
		cmd->argument = calloc(len, sizeof(char));
		// append...
		strncpy(cmd->argument, data, len);
	}

	if (cmd->argumentsExpected == 0 || (cmd->argumentsExpected == 1 && cmd->argument != NULL)) {
		if (strcmp(cmd->commandName, "Visit") == 0) {
			cef_string_t some_url = {};
			cef_string_utf8_to_utf16(cmd->argument, strlen(cmd->argument), &some_url);
			cef_frame_t *frame = client->browser->get_main_frame(client->browser);
			client->on_load_end = handle_load_event;
			frame->load_url(frame, &some_url);
		} else if (strcmp(cmd->commandName, "Body") == 0) {
			cef_string_visitor_t *visitor;
			visitor = calloc(1, sizeof(cef_string_visitor_t));
			visitor->base.size = sizeof(cef_string_visitor_t);
			initialize_cef_base((cef_base_t*)visitor);
			visitor->visit = get_frame_source;
			cef_frame_t *frame = client->browser->get_main_frame(client->browser);
			frame->get_source(frame, visitor);
		}

		cmd->argumentsExpected = -1;
		cmd->commandName = NULL;
	}
}

void processNext(ReceivedCommand *cmd, const char *data, client_t *client, int *expectingDataSize) {
	if (cmd->commandName == NULL) {
		int len = strlen(data) + 1;
		cmd->commandName = calloc(len, sizeof(char));
		strncpy(cmd->commandName, data, len);
		cmd->argumentsExpected = -1;
	} else {
		processArgument(cmd, data, client, expectingDataSize);
	}
}

void
checkNext(client_t *client, ReceivedCommand *cmd, int *expectingDataSize)
{
	if (*expectingDataSize == -1) {
		// readLine
		char buffer[128];
		fgets(buffer, sizeof(buffer), stdin);
		if (feof(stdin))
		    return;
		buffer[strlen(buffer) - 1] = 0;

		processNext(cmd, buffer, client, expectingDataSize);

		checkNext(client, cmd, expectingDataSize);
	} else {
		// readDataBlock
		char otherBuffer[*expectingDataSize + 1];
		int bytesRead;
		bytesRead = fread(otherBuffer, 1, *expectingDataSize, stdin);
		if (bytesRead != *expectingDataSize)
			return;
		otherBuffer[*expectingDataSize] = 0;

		processNext(cmd, otherBuffer, client, expectingDataSize);

		*expectingDataSize = -1;
		checkNext(client, cmd, expectingDataSize);
	}
}

void *f(void *arg) {
	client_t *client = (client_t *)arg;
	while (!feof(stdin)) {
		ReceivedCommand *cmd;
		cmd = calloc(1, sizeof(ReceivedCommand));
		cmd->commandName = NULL;
		int expectingDataSize = -1;

		checkNext(client, cmd, &expectingDataSize);
	}

	cef_browser_host_t *host = client->browser->get_host(client->browser);
	host->close_browser(host, 1);

	return NULL;
}

int main(int argc, char** argv) {
    // Main args.
    cef_main_args_t mainArgs = {};
    mainArgs.argc = argc;
    mainArgs.argv = argv;
    
    // Application handler and its callbacks.
    // cef_app_t structure must be filled. It must implement
    // reference counting. You cannot pass a structure 
    // initialized with zeroes.
    cef_app_t app = {};
    initialize_app_handler(&app);
    
    // Execute subprocesses.
    fprintf(stderr, "cef_execute_process, argc=%d\n", argc);
    int code = cef_execute_process(&mainArgs, &app, NULL);
    if (code >= 0) {
        _exit(code);
    }
    
    // Application settings.
    // It is mandatory to set the "size" member.
    cef_settings_t settings = {};
    settings.size = sizeof(cef_settings_t);
    settings.no_sandbox = 1;

    // Initialize CEF.
    fprintf(stderr, "cef_initialize\n");
    cef_initialize(&mainArgs, &settings, &app, NULL);

    cef_window_info_t windowInfo = {};
#ifndef WINDOWLESS
    // Create GTK window. You can pass a NULL handle 
    // to CEF and then it will create a window of its own.
    initialize_gtk();
    GtkWidget* hwnd = create_gtk_window("cefcapi example", 1024, 768);
    windowInfo.parent_window = gdk_x11_drawable_get_xid(gtk_widget_get_window(hwnd));
    // windowInfo.parent_window = hwnd;
#else
    windowInfo.windowless_rendering_enabled = 1;
#endif
    
    // Browser settings.
    // It is mandatory to set the "size" member.
    cef_browser_settings_t browserSettings = {};
    browserSettings.size = sizeof(cef_browser_settings_t);
    
    // Client handler and its callbacks.
    // cef_client_t structure must be filled. It must implement
    // reference counting. You cannot pass a structure 
    // initialized with zeroes.
    client_t client = {};
    client.browser = NULL;
    client.on_load_end = NULL;
    initialize_client_handler((cef_client_t *)&client);

    // Create browser.
    fprintf(stderr, "cef_browser_host_create_browser\n");
    cef_browser_host_create_browser(&windowInfo, (cef_client_t *)&client, NULL,
            &browserSettings, NULL);

    pthread_t pth;
    pthread_create(&pth, NULL, f, &client);

    // Message loop.
    fprintf(stderr, "cef_run_message_loop\n");
    cef_run_message_loop();

    // Shutdown CEF.
    fprintf(stderr, "cef_shutdown\n");
    cef_shutdown();

    return 0;
}

static void ready() {
    printf("Ready\n");
    fflush(stdout);
}
