#include <stddef.h>
typedef struct duk_enclave_script {
	const char *key;
	const char *start;
	const char *end;
} duk_enclave_script_t;
#ifdef __cplusplus
extern "C" {
#endif
extern const size_t MAX_SCRIPT;
extern const duk_enclave_script_t SCRIPTS[];
extern const duk_enclave_script_t *AUTOEXEC_SCRIPT;
#ifdef __cplusplus
}
#endif
