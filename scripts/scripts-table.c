#include "scripts.h"
extern const char binary_main_js_size[], binary_main_js_start[], binary_main_js_end[];
extern const char binary_framework_js_size[], binary_framework_js_start[], binary_framework_js_end[];
extern const char binary_Promise_js_size[], binary_Promise_js_start[], binary_Promise_js_end[];
const size_t MAX_SCRIPT = 3;
const duk_enclave_script_t SCRIPTS[3] = {
	{"main.js", (size_t) binary_main_js_size, binary_main_js_start, binary_main_js_end},
	{"framework.js", (size_t) binary_framework_js_size, binary_framework_js_start, binary_framework_js_end},
	{"Promise.js", (size_t) binary_Promise_js_size, binary_Promise_js_start, binary_Promise_js_end},
};
