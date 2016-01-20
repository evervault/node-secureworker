This is duktape in an enclave, in Node.js.

## Build instructions
First, run
```sh
openssl.exe genrsa -out duk_enclave\duk_enclave_private.pem -3 3072
```

(The SGX SDK puts openssl.exe in `%PATH%`.)
(Normally the SGX SDK does this for you when you create an enclave project.)

Enter the JavaScript files in `duk_enclave/scripts.h`.

Then you can build it in Visual Studio.

## Usage
```js
var SecureWorkerInternal = require('./Release/secureworker_internal');
// Pass the location of the enclave DLL.
var wi = new SecureWorkerInternal('./Debug/duk_enclave.signed.dll');
wi.handlePostMesage = function (message) {
  console.log('from wi:', message);
};
// Pass the index of the script to run.
wi.init(0);
wi.emitMessage('hello from outside');
wi.close();
// The enclave continues to exist until the SecureWorkerInternal instance is garbage collected.
wi = null;
```

## TODO
- port changes to original duktape source (this code is modified from the release tarball)
- wrap code changes in a new config flag or something

## Important configuration
### duktape-dist
- SGX include dirs, `/NODEFAULTLIB` - Compile for SGX platform.
- `DUK_OPT_NO_FILE_IO` - File I/O is not available in enclave.
- `DUK_OPT_CPP_EXCEPTIONS` - The SGX SDK doesn't support `setjmp`.
- `DUK_OPT_NO_JX` - The SGX SDK doesn't support `sscanf`, which JX needs.
- `/TP` - Compile as C++ because we need C++ exceptions.

### sample-client
- Working Directory set to `$(OutDir)` - The enclave dll is there.

### node-secureworker-internal
- Use Release configuration, because node-gyp doesn't download Debug libraries.

## Code changes (duktape-dist)
- `DUK_SNPRINTF` in duk_bi_date.c - The SGX SDK doesn't support `sprintf` (without the "n"). Provide `DUK_BI_DATE_ISO8601_BUFSIZE`.
- `duk_bi_date_get_*_sgx` - Stubbed out routines for getting the time and locale information from the OS. Consider using SGX's Trusted Time system to implement this for real.
- Don't include `<windows.h>` - We're not running on Windows.
- Don't include `<setjmp.h>` - The SGX SDK doesn't have this file.
- Use unprefixed `snprintf` and `vsnprintf` - That's what the SGX SDK provides.
- Remove `duk_file` and `DUK_STD*` - The SGX SDK doesn't provide the underlying `FILE` and `std*`.
