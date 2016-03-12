#include "scripts.h"
#ifdef _WIN32
#define BINARY(NAME) binary_ ## NAME
#else
#define BINARY(NAME) _binary_ ## NAME
#endif
extern const char BINARY(main_js_start)[], BINARY(main_js_end)[];
extern const char BINARY(framework_js_start)[], BINARY(framework_js_end)[];
extern const char BINARY(Promise_js_start)[], BINARY(Promise_js_end)[];
const size_t MAX_SCRIPT = 3;
const duk_enclave_script_t SCRIPTS[3] = {
	{"main.js", BINARY(main_js_start), BINARY(main_js_end)},
	{"framework.js", BINARY(framework_js_start), BINARY(framework_js_end)},
	{"Promise.js", BINARY(Promise_js_start), BINARY(Promise_js_end)},
};
