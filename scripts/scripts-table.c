#include "scripts.h"
#ifdef _WIN32
#define BINARY(NAME) binary_ ## NAME
#else
#define BINARY(NAME) _binary_ ## NAME
#endif
extern const char BINARY(framework_autoexec_js_start)[], BINARY(framework_autoexec_js_end)[];
extern const char BINARY(framework_Promise_js_start)[], BINARY(framework_Promise_js_end)[];
extern const char BINARY(framework_framework_js_start)[], BINARY(framework_framework_js_end)[];
extern const char BINARY(main_js_start)[], BINARY(main_js_end)[];
const size_t MAX_SCRIPT = 4;
const duk_enclave_script_t SCRIPTS[4] = {
	{"../framework/autoexec.js", BINARY(framework_autoexec_js_start), BINARY(framework_autoexec_js_end)},
	{"../framework/Promise.js", BINARY(framework_Promise_js_start), BINARY(framework_Promise_js_end)},
	{"../framework/framework.js", BINARY(framework_framework_js_start), BINARY(framework_framework_js_end)},
	{"main.js", BINARY(main_js_start), BINARY(main_js_end)},
};
const duk_enclave_script_t *AUTOEXEC_SCRIPT = &SCRIPTS[0];
