#include <sstream>
#include <iomanip>
#include <node.h>
#include <tchar.h>
#include "sgx_urts.h"
#include "build/duk_enclave_u.h"

// TODO: Create multiple instances

void throw_with_status(const char *message, const sgx_status_t status) {
	std::stringstream ss;
	ss << message << " (0x" << std::hex << std::setw(4) << std::setfill('0') << status << ")";
	v8::ThrowException(v8::Exception::Error(v8::String::New(ss.str().c_str())));
}

sgx_enclave_id_t enclave_id;
v8::Persistent<v8::Object> handlers;

v8::Handle<v8::Value> native_emit_message(const v8::Arguments& args) {
	v8::HandleScope scope;
	if (!args[0]->IsString()) {
		v8::ThrowException(v8::Exception::TypeError(v8::String::New("Argument error")));
		return scope.Close(v8::Undefined());
	}
	v8::String::Utf8Value arg0_utf8(args[0]);
	const char *message = *arg0_utf8;
	{
		const sgx_status_t status = duk_enclave_emit_message(enclave_id, message);
		if (status != SGX_SUCCESS) {
			throw_with_status("duk_enclave_emit_message failed", status);
			return scope.Close(v8::Undefined());
		}
	}
	return scope.Close(v8::Undefined());
}

void duk_enclave_post_message(const char *message) {
	v8::HandleScope scope;
	v8::Local<v8::Value> args[] = {v8::String::New(message)};
	handlers->Get(v8::String::NewSymbol("postMessage")).As<v8::Function>()->Call(handlers, 1, args);
}

static void secureworker_init(v8::Handle<v8::Object> exports) {
	v8::Local<v8::Object> native = v8::Object::New();
	NODE_SET_METHOD(native, "emitMessage", native_emit_message);
	exports->Set(v8::String::NewSymbol("native"), native);
	handlers = v8::Persistent<v8::Object>::New(v8::Object::New());
	exports->Set(v8::String::NewSymbol("handlers"), handlers);
	{
		sgx_launch_token_t launch_token;
		int launch_token_updated;
		const sgx_status_t status = sgx_create_enclave(_T("duk_enclave.signed.dll"), SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &enclave_id, NULL);
		if (status != SGX_SUCCESS) {
			throw_with_status("sgx_create_enclave failed", status);
			return;
		}
	}
	{
		const sgx_status_t status = duk_enclave_init(enclave_id, 0);
		if (status != SGX_SUCCESS) {
			throw_with_status("duk_enclave_init failed", status);
			return;
		}
	}
}

// TODO: Do modules have to clean up after themselves at some point? We should destroy the enclave.
static void secureworker_close() {
	{
		const sgx_status_t status = duk_enclave_close(enclave_id);
		if (status != SGX_SUCCESS) {
			throw_with_status("duk_enclave_close failed", status);
			return;
		}
	}
	{
		const sgx_status_t status = sgx_destroy_enclave(enclave_id);
		if (status != SGX_SUCCESS) {
			throw_with_status("sgx_destroy_enclave failed", status);
			return;
		}
	}
}

NODE_MODULE(secureworker_internal, secureworker_init);
