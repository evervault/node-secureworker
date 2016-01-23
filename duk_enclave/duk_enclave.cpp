#include "duk_enclave_t.h"

#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include "duktape.h"

#include "scripts.h"

static duk_ret_t native_post_message(duk_context *ctx) {
	const char * const message = duk_get_string(ctx, 0);
	if (message == NULL) return DUK_RET_TYPE_ERROR;
	{
		const sgx_status_t status = duk_enclave_post_message(message);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	return 0;
}

static duk_ret_t native_next_tick(duk_context *ctx) {
	if (!duk_is_function(ctx, 0)) return DUK_RET_TYPE_ERROR;
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, "microtasks");
	duk_push_string(ctx, "push");
	// [arg0] [heap_stash] [heap_stash.microtasks] ["push"]
	duk_dup(ctx, 0);
	duk_call_prop(ctx, -3, 1);
	return 0;
}

static duk_ret_t native_import_script(duk_context *ctx) {
	if (!duk_is_number(ctx, 0)) return DUK_RET_TYPE_ERROR;
	const int key = duk_get_int(ctx, 0);
	if (key < 0 || key >= MAX_SCRIPT) return DUK_RET_RANGE_ERROR;
	duk_eval_string_noresult(ctx, SCRIPTS[key]);
	return 0;
}

static duk_ret_t native_encode_string(duk_context *ctx) {
	duk_size_t in_size;
	const char * const in = duk_get_lstring(ctx, 0, &in_size);
	if (in == NULL) return DUK_RET_TYPE_ERROR;
	duk_push_fixed_buffer(ctx, in_size);
	memcpy(duk_get_buffer_data(ctx, -1, NULL), in, in_size);
	duk_push_buffer_object(ctx, -1, 0, in_size, DUK_BUFOBJ_UINT8ARRAY);
	return 1;
}

static duk_ret_t native_decode_string(duk_context *ctx) {
	duk_size_t in_size;
	const void * const in = duk_get_buffer_data(ctx, 0, &in_size);
	if (in == NULL) return DUK_RET_TYPE_ERROR;
	duk_push_lstring(ctx, reinterpret_cast<const char *>(in), in_size);
	return 1;
}

static duk_ret_t native_sha256_digest(duk_context *ctx) {
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 0, &data_size);
	if (data == NULL) return DUK_RET_TYPE_ERROR;
	duk_push_fixed_buffer(ctx, SGX_SHA256_HASH_SIZE);
	void *result = duk_get_buffer_data(ctx, -1, NULL);
	{
		const sgx_status_t status = sgx_sha256_msg(reinterpret_cast<const uint8_t *>(data), data_size, reinterpret_cast<sgx_sha256_hash_t *>(result));
		if (status != SGX_SUCCESS) abort();
	}
	duk_push_buffer_object(ctx, -1, 0, SGX_SHA256_HASH_SIZE, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static const duk_function_list_entry native_methods[] = {
	{"postMessage", native_post_message, 1},
	{"nextTick", native_next_tick, 1},
	{"importScript", native_import_script, 1},
	{"encodeString", native_encode_string, 1},
	{"decodeString", native_decode_string, 1},
	{"sha256Digest", native_sha256_digest, 1},
	{NULL, NULL, 0},
};

static void spin_microtasks(duk_context *ctx) {
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, "microtasks");
	duk_get_prop_string(ctx, -1, "shift");
	// [heap_stash] [heap_stash.microtasks] [heap_stash.microtasks.shift]
	for (;;) {
		duk_dup(ctx, -1);
		duk_dup(ctx, -3);
		duk_call_method(ctx, 0);
		if (duk_is_undefined(ctx, -1)) break;
		duk_pcall(ctx, 0);
		duk_pop(ctx);
	}
	duk_pop_3(ctx);
}

static duk_context *ctx = NULL;

void duk_enclave_init(int key) {
	if (ctx != NULL) abort();
	if (key < 0 || key >= MAX_SCRIPT) abort();
	ctx = duk_create_heap_default();
	if (ctx == NULL) abort();
	// Framework code
	duk_push_heap_stash(ctx);
	duk_push_array(ctx);
	duk_put_prop_string(ctx, -2, "microtasks");
	duk_pop(ctx);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, native_methods);
	duk_put_global_string(ctx, "_dukEnclaveNative");
	duk_push_object(ctx);
	duk_put_global_string(ctx, "_dukEnclaveHandlers");
	// Application code
	duk_eval_string_noresult(ctx, SCRIPTS[key]);
	spin_microtasks(ctx);
}

void duk_enclave_close() {
	if (ctx == NULL) abort();
	duk_destroy_heap(ctx);
	ctx = NULL;
}

void duk_enclave_emit_message(const char *message) {
	if (ctx == NULL) abort();
	duk_get_global_string(ctx, "_dukEnclaveHandlers");
	duk_get_prop_string(ctx, -1, "emitMessage");
	duk_push_string(ctx, message);
	duk_pcall(ctx, 1);
	duk_pop(ctx);
	spin_microtasks(ctx);
}
