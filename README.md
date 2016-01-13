This is duktape in an enclave.

TODO:
- .gitignore and remove unnecessary files from repo
- port changes to original duktape source (this code is modified from the release tarball)
- wrap code changes in a new config flag or something

## Important configuration
- SGX include dirs, `/NODEFAULTLIB` - Compile for SGX platform.
- `DUK_OPT_NO_FILE_IO` - File I/O is not available in enclave.
- `DUK_OPT_CPP_EXCEPTIONS` - The SGX SDK doesn't support `setjmp`.
- `DUK_OPT_NO_JX` - The SGX SDK doesn't support `sscanf`, which JX needs.
- `/TP` - Compile as C++ because we need C++ exceptions.

## Code changes
- `DUK_SNPRINTF` in duk_bi_date.c - The SGX SDK doesn't support `sprintf` (without the "n"). Provide `DUK_BI_DATE_ISO8601_BUFSIZE`.
- `duk_bi_date_get_*_sgx` - Stubbed out routines for getting the time and locale information from the OS. Consider using SGX's Trusted Time system to implement this for real.
- Don't include `<windows.h>` - We're not running on Windows.
- Don't include `<setjmp.h>` - The SGX SDK doesn't have this file.
- Use unprefixed `snprintf` and `vsnprintf` - That's what the SGX SDK provides.
- Remove `duk_file` and `DUK_STD*` - The SGX SDK doesn't provide the underlying `FILE` and `std*`.
