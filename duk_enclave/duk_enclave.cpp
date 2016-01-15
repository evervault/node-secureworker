#include "duk_enclave_t.h"

#include "sgx_trts.h"
#include "duktape.h"

static duk_ret_t f_post_message(duk_context *ctx) {
	const char * const message = duk_get_string(ctx, 0);
	if (message == NULL) return DUK_RET_TYPE_ERROR;
	duk_enclave_post_message(message);
	return 0;
}

const duk_function_list_entry f_methods[] = {
	{"postMessage", f_post_message, 1},
	{NULL, NULL, 0},
};

duk_context *ctx = NULL;

void duk_enclave_init() {
	ctx = duk_create_heap_default();
	duk_eval_string(ctx,
		"var F = {};\n"
		"var i = 0;\n"
		"F._handlers.emitMessage = function (message) {\n"
		"	F.postMessage(i + ': ' + message);\n"
		"	i++;\n"
		"};\n"
	);
	duk_pop(ctx);
	duk_eval_string(ctx, "F");
	duk_put_function_list(ctx, -1, f_methods);
	duk_pop(ctx);
}

void duk_enclave_close() {
	duk_destroy_heap(ctx);
}

void duk_enclave_emit_message(const char *message) {
	duk_eval_string(ctx, "F._handlers.emitMessage");
	duk_push_string(ctx, message);
	duk_call(ctx, 1);
	duk_pop(ctx);
}
