#include <stddef.h>
typedef struct duk_enclave_script {
	size_t size;
	const char * start;
	const char * end;
} duk_enclave_script_t;
#ifdef __cplusplus
extern "C" {
#endif
extern const size_t MAX_SCRIPT;
extern const duk_enclave_script_t SCRIPTS[];
#ifdef __cplusplus
}
#endif
