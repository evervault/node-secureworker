#include <iostream>
#include <node.h>
#include <tchar.h>
#include "sgx_urts.h"
#include "build/duktape-enclave_u.h"

sgx_enclave_id_t enclave_id;

v8::Handle<v8::Value> test(const v8::Arguments& args) {
	v8::HandleScope scope;
	double result;
	{
		const sgx_status_t status = duk_enclave_test(enclave_id, &result);
		if (status != SGX_SUCCESS) {
			v8::ThrowException(v8::Exception::Error(v8::String::New("duk_enclave_test failed")));
			return scope.Close(v8::Undefined());
		}
	}
	std::cout << "result: " << result << std::endl;
	return scope.Close(v8::Number::New(result));
}

static void init(v8::Local<v8::Object> exports) {
	{
		sgx_launch_token_t launch_token;
		int launch_token_updated;
		const sgx_status_t status = sgx_create_enclave(_T("duktape-enclave.signed.dll"), SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &enclave_id, NULL);
		if (status != SGX_SUCCESS) {
			v8::ThrowException(v8::Exception::Error(v8::String::New("sgx_create_enclave failed")));
			return;
		}
	}
	{
		const sgx_status_t status = duk_enclave_init(enclave_id);
		if (status != SGX_SUCCESS) {
			v8::ThrowException(v8::Exception::Error(v8::String::New("duk_enclave_init failed")));
			return;
		}
	}
	NODE_SET_METHOD(exports, "test", test);
}

// TODO: Do modules have to clean up after themselves at some point? We should destroy the enclave.
static void close() {
	duk_enclave_close(enclave_id);
	sgx_destroy_enclave(enclave_id);
}

NODE_MODULE(m, init);
