#include <node.h>

#include <iomanip>
#include <iostream> // %%%
#include <sstream>
#include <tchar.h>

#include "sgx_urts.h"
#include "build/duk_enclave_u.h"

static struct entry_info;

__declspec(thread) entry_info *thread_entry = nullptr;

static struct entry_info {
	entry_info *previous;
	v8::Handle<v8::Object> entrant;
	entry_info(v8::Handle<v8::Object> entrant) : previous(thread_entry), entrant(entrant) {
		thread_entry = this;
	}
	~entry_info() {
		assert(thread_entry == this);
		thread_entry = previous;
	}
};

static void throw_with_status(const char *message, const sgx_status_t status) {
	std::stringstream ss;
	ss << message << " (0x" << std::hex << std::setw(4) << std::setfill('0') << status << ")";
	v8::ThrowException(v8::Exception::Error(v8::String::New(ss.str().c_str())));
}

static class SecureWorkerInternal : public node::ObjectWrap {
public:
	sgx_enclave_id_t enclave_id;
	explicit SecureWorkerInternal(const char *file_name);
	~SecureWorkerInternal();
	void init(int key);
	void close();
	void emitMessage(const char *message);
	static v8::Handle<v8::Value> New(const v8::Arguments &arguments);
	static v8::Handle<v8::Value> Init(const v8::Arguments &arguments);
	static v8::Handle<v8::Value> Close(const v8::Arguments &arguments);
	static v8::Handle<v8::Value> EmitMessage(const v8::Arguments &arguments);
};

SecureWorkerInternal::SecureWorkerInternal(const char *file_name) : enclave_id(0) {
	sgx_launch_token_t launch_token;
	int launch_token_updated;
	const sgx_status_t status = sgx_create_enclave(file_name, SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &enclave_id, NULL);
	if (status != SGX_SUCCESS) throw status;
	std::cerr << "created enclave " << enclave_id << std::endl; // %%%
}

SecureWorkerInternal::~SecureWorkerInternal() {
	const sgx_status_t status = sgx_destroy_enclave(enclave_id);
	if (status != SGX_SUCCESS) throw status;
	std::cerr << "destroyed enclave " << enclave_id << std::endl; // %%%
	enclave_id = 0;
}

void SecureWorkerInternal::init(int key) {
	const sgx_status_t status = duk_enclave_init(enclave_id, key);
	if (status != SGX_SUCCESS) throw status;
}

void SecureWorkerInternal::close() {
	const sgx_status_t status = duk_enclave_close(enclave_id);
	if (status != SGX_SUCCESS) throw status;
}

void SecureWorkerInternal::emitMessage(const char *message) {
	const sgx_status_t status = duk_enclave_emit_message(enclave_id, message);
	if (status != SGX_SUCCESS) throw status;
}

v8::Handle<v8::Value> SecureWorkerInternal::New(const v8::Arguments &arguments) {
	v8::HandleScope scope;
	if (!arguments.IsConstructCall()) {
		v8::ThrowException(v8::Exception::Error(v8::String::New("SecureWorkerInternal called not as a constructor")));
		return scope.Close(v8::Undefined());
	}
	if (!arguments[0]->IsString()) {
		v8::ThrowException(v8::Exception::TypeError(v8::String::New("Argument error")));
		return scope.Close(v8::Undefined());
	}
	v8::String::Utf8Value arg0_utf8(arguments[0]);
	SecureWorkerInternal *secure_worker_internal;
	try {
		entry_info entry(arguments.This());
		secure_worker_internal = new SecureWorkerInternal(*arg0_utf8);
	} catch (sgx_status_t status) {
		throw_with_status("sgx_create_enclave failed", status);
		return scope.Close(v8::Undefined());
	}
	secure_worker_internal->Wrap(arguments.This());
	// Why doesn't this need scope.Close?
	return arguments.This();
}

v8::Handle<v8::Value> SecureWorkerInternal::Init(const v8::Arguments &arguments) {
	v8::HandleScope scope;
	if (!arguments[0]->IsNumber()) {
		v8::ThrowException(v8::Exception::TypeError(v8::String::New("Argument error")));
		return scope.Close(v8::Undefined());
	}
	int key = arguments[0]->NumberValue();
	SecureWorkerInternal *secure_worker_internal = node::ObjectWrap::Unwrap<SecureWorkerInternal>(arguments.This());
	try {
		entry_info entry(arguments.This());
		secure_worker_internal->init(key);
	} catch (sgx_status_t status) {
		throw_with_status("duk_enclave_init failed", status);
		return scope.Close(v8::Undefined());
	}
	return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> SecureWorkerInternal::Close(const v8::Arguments &arguments) {
	v8::HandleScope scope;
	SecureWorkerInternal *secure_worker_internal = node::ObjectWrap::Unwrap<SecureWorkerInternal>(arguments.This());
	try {
		entry_info entry(arguments.This());
		secure_worker_internal->close();
	} catch (sgx_status_t status) {
		throw_with_status("duk_enclave_close failed", status);
		return scope.Close(v8::Undefined());
	}
	return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> SecureWorkerInternal::EmitMessage(const v8::Arguments &arguments) {
	v8::HandleScope scope;
	if (!arguments[0]->IsString()) {
		v8::ThrowException(v8::Exception::TypeError(v8::String::New("Argument error")));
		return scope.Close(v8::Undefined());
	}
	v8::String::Utf8Value arg0_utf8(arguments[0]);
	SecureWorkerInternal *secure_worker_internal = node::ObjectWrap::Unwrap<SecureWorkerInternal>(arguments.This());
	try {
		entry_info entry(arguments.This());
		secure_worker_internal->emitMessage(*arg0_utf8);
	} catch (sgx_status_t status) {
		throw_with_status("duk_enclave_emit_message failed", status);
		return scope.Close(v8::Undefined());
	}
	return scope.Close(v8::Undefined());
}

static void secureworker_internal_init(v8::Handle<v8::Object> exports, v8::Handle<v8::Object> module) {
	v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(SecureWorkerInternal::New);
	function_template->SetClassName(v8::String::NewSymbol("SecureWorkerInternal"));
	function_template->InstanceTemplate()->SetInternalFieldCount(1);
	function_template->PrototypeTemplate()->Set(v8::String::NewSymbol("init"), v8::FunctionTemplate::New(SecureWorkerInternal::Init)->GetFunction());
	function_template->PrototypeTemplate()->Set(v8::String::NewSymbol("close"), v8::FunctionTemplate::New(SecureWorkerInternal::Close)->GetFunction());
	function_template->PrototypeTemplate()->Set(v8::String::NewSymbol("emitMessage"), v8::FunctionTemplate::New(SecureWorkerInternal::EmitMessage)->GetFunction());
	function_template->PrototypeTemplate()->Set(v8::String::NewSymbol("handlePostMessage"), v8::Null());
	module->Set(v8::String::NewSymbol("exports"), function_template->GetFunction());
}

void duk_enclave_post_message(const char *message) {
	v8::HandleScope scope;
	assert(thread_entry != nullptr);
	v8::Local<v8::Value> handle_post_message = thread_entry->entrant->Get(v8::String::NewSymbol("handlePostMessage"));
	if (!handle_post_message->IsFunction()) return;
	v8::Local<v8::Value> arguments[] = {v8::String::New(message)};
	handle_post_message.As<v8::Function>()->Call(thread_entry->entrant, 1, arguments);
}

NODE_MODULE(secureworker_internal, secureworker_internal_init);
