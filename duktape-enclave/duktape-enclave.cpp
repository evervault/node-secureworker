#include "duktape-enclave_t.h"

#include "sgx_trts.h"
#include "duktape.h"

double test() {
	duk_context *ctx = duk_create_heap_default();
	duk_eval_string(ctx, "9 * 8");
	const double result = duk_to_number(ctx, -1);
	duk_pop(ctx);
	duk_destroy_heap(ctx);
	return result;
}
