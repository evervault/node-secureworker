#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>

#include "../duktape-1.4.0/src/duktape.h"
#include "sunspider-scripts-only.h"

int main(int argc, char* argv[]) {
	std::cout << std::fixed << std::setprecision(2);
	for (size_t i = 0; i < MAX_SCRIPT; i++) {
		duk_context *ctx = duk_create_heap_default();
		assert(ctx != nullptr);
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		duk_eval_string_noresult(ctx, SCRIPTS[i]);
		std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
		duk_destroy_heap(ctx);
		std::chrono::duration<double, std::micro> diff = finish - start;
		std::cout << diff.count() << " us" << std::endl;
	}
}
