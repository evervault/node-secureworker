#include "scripts.h"
// TODO: generate this
extern "C" {
	extern const char binary_main_js_start[], binary_main_js_end[];
	extern const char binary_framework_js_start[], binary_framework_js_end[];
}
const size_t MAX_SCRIPT = 2;
const duk_enclave_script SCRIPTS[MAX_SCRIPT] = {
	{binary_main_js_start, binary_main_js_end - binary_main_js_start},
	{binary_framework_js_start, binary_framework_js_end - binary_framework_js_start},
};
