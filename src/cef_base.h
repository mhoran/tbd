// Copyright (c) 2014 The cefcapi authors. All rights reserved.
// License: BSD 3-clause.
// Website: https://github.com/CzarekTomczak/cefcapi

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "include/capi/cef_base_capi.h"
#include "include/capi/cef_app_capi.h"

// Print only the first execution of the callback,
// ignore the subsequent.
#define DEBUG_CALLBACK(x) { static int first_call = 1; if (first_call == 1) { first_call = 0; fprintf(stderr, x); } }

// ----------------------------------------------------------------------------
// cef_base_t
// ----------------------------------------------------------------------------

///
// Structure defining the reference count implementation functions. All
// framework structures must include the cef_base_t structure first.
///

///
// Increment the reference count.
///
#define ADD_REF(type) \
static \
void \
CEF_CALLBACK \
type##_add_ref(cef_base_t *self) \
{ \
	struct _##type *handler = (struct _##type *)self; \
	atomic_fetch_add(&handler->ref_count, 1); \
}

///
// Decrement the reference count.  Delete this object when no references
// remain.
///
#define RELEASE(type) \
static \
int \
CEF_CALLBACK \
type##_release(cef_base_t* self) { \
	struct _##type *handler = (struct _##type *)self; \
	if (atomic_fetch_sub(&handler->ref_count, 1) - 1 == 0) { \
		free(handler); \
		return 1; \
	} \
	return 0; \
}

///
// Returns the current number of references.
///
#define HAS_ONE_REF(type) \
static \
int \
CEF_CALLBACK \
type##_has_one_ref(cef_base_t* self) { \
	struct _##type *handler = (struct _##type *)self; \
	return atomic_load(&handler->ref_count) == 1; \
}

#define IMPLEMENT_REFCOUNTING(type) \
ADD_REF(type) \
RELEASE(type) \
HAS_ONE_REF(type)

#define GENERATE_CEF_BASE_INITIALIZER(type) \
void \
initialize_##type##_base(type *object) \
{ \
	cef_base_t *base = (cef_base_t *)object; \
	base->size = sizeof(type); \
	base->add_ref = type##_add_ref; \
	base->release = type##_release; \
	base->has_one_ref = type##_has_one_ref; \
}

struct _life_span_handler_t;
struct _client_t;
struct _render_process_handler;
struct _load_handler;
struct _render_handler;
struct _string_visitor;

void initialize_life_span_handler_t_base(struct _life_span_handler_t *object);
void initialize_client_t_base(struct _client_t *object);
void initialize_render_process_handler_base(struct _render_process_handler *object);
void initialize_load_handler_base(struct _load_handler *object);
void initialize_render_handler_base(struct _render_handler *object);
void initialize_string_visitor_base(struct _string_visitor *object);

#define initialize_cef_base(T) \
    _Generic((T), \
	struct _life_span_handler_t*: initialize_life_span_handler_t_base, \
	struct _client_t*: initialize_client_t_base, \
	struct _render_process_handler*: initialize_render_process_handler_base, \
	struct _load_handler*: initialize_load_handler_base, \
	struct _render_handler*: initialize_render_handler_base, \
	struct _string_visitor*: initialize_string_visitor_base)(T)
