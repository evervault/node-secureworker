#include <iostream>
#include <node.h>
#include <tchar.h>
#include "sgx_urts.h"
#include "build/duktape-enclave_u.h"

sgx_enclave_id_t enclave_id;
v8::Persistent<v8::Object> handlers;

v8::Handle<v8::Value> f_emit_message(const v8::Arguments& args) {
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
			// TODO: Include the status code when throwing.
			v8::ThrowException(v8::Exception::Error(v8::String::New("duk_enclave_emit_message failed")));
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

static void m_init(v8::Handle<v8::Object> exports) {
	handlers = v8::Persistent<v8::Object>::New(v8::Object::New());
	NODE_SET_METHOD(exports, "_emitMessage", f_emit_message);
	exports->Set(v8::String::NewSymbol("_handlers"), handlers);
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
}

// TODO: Do modules have to clean up after themselves at some point? We should destroy the enclave.
static void m_close() {
	{
		const sgx_status_t status = duk_enclave_close(enclave_id);
		if (status != SGX_SUCCESS) {
			v8::ThrowException(v8::Exception::Error(v8::String::New("duk_enclave_close failed")));
			return;
		}
	}
	{
		const sgx_status_t status = sgx_destroy_enclave(enclave_id);
		if (status != SGX_SUCCESS) {
			v8::ThrowException(v8::Exception::Error(v8::String::New("sgx_destroy_enclave failed")));
			return;
		}
	}
}

NODE_MODULE(m, m_init);
