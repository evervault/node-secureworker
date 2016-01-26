#include <stddef.h>
struct duk_enclave_script {
	const char * start;
	size_t size;
};
extern const size_t MAX_SCRIPT;
extern const duk_enclave_script SCRIPTS[];
