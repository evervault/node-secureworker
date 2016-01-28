#include "scripts.h"
extern const char binary_main_js_size[], binary_main_js_start[], binary_main_js_end[];
extern const char binary_framework_js_size[], binary_framework_js_start[], binary_framework_js_end[];
const size_t MAX_SCRIPT = 2;
const duk_enclave_script_t SCRIPTS[2] = {
	{"main.js", (size_t) binary_main_js_size, binary_main_js_start, binary_main_js_end},
	{"framework.js", (size_t) binary_framework_js_size, binary_framework_js_start, binary_framework_js_end},
};
