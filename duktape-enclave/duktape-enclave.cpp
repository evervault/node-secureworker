#include "duktape-enclave_t.h"

#include "sgx_trts.h"
#include "duktape.h"

duk_context *ctx = NULL;

void duk_enclave_init() {
	ctx = duk_create_heap_default();
}

void duk_enclave_close() {
	duk_destroy_heap(ctx);
}

double duk_enclave_test() {
	duk_eval_string(ctx, "9 * 8");
	const double result = duk_to_number(ctx, -1);
	duk_pop(ctx);
	return result;
}
