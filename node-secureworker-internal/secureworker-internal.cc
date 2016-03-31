#include <node.h>
#include <v8_typed_array.h>

#include <iomanip>
#include <iostream> // %%%
#include <sstream>

#include "sgx_tseal.h"
#include "sgx_uae_service.h"
#include "sgx_urts.h"
#include "duk_enclave_u.h"

// Track the "current" ECALL's associated Object so that OCALLS can find the callback.

struct entry_info;

#ifdef _WIN32
__declspec(thread)
#else
__thread
#endif
entry_info *thread_entry = nullptr;

struct entry_info {
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

// Convenience class for communicating SGX statuses to v8 exceptions

struct sgx_error {
	sgx_status_t status;
	const char *source;
	sgx_error(sgx_status_t status, const char *source) : status(status), source(source) {
	}
	void rethrow() {
		std::stringstream ss;
		ss << source << " failed (0x" << std::hex << std::setw(4) << std::setfill('0') << status << ")";
		v8::ThrowException(v8::Exception::Error(v8::String::New(ss.str().c_str())));
	}
};

// The rest of the stuff, which is per-instance

class SecureWorkerInternal : public node::ObjectWrap {
public:
	sgx_enclave_id_t enclave_id;
	explicit SecureWorkerInternal(const char *file_name);
	~SecureWorkerInternal();
	void init(const char *key);
	void close();
	void emitMessage(const char *message);
	void bootstrapMock(sgx_sealed_data_t *out, size_t out_size,
	                   const uint8_t *additional_data, size_t additional_data_size,
	                   const uint8_t *clear, size_t data_size);
	static v8::Handle<v8::Value> New(const v8::Arguments &arguments);
	static v8::Handle<v8::Value> Init(const v8::Arguments &arguments);
	static v8::Handle<v8::Value> Close(const v8::Arguments &arguments);
	static v8::Handle<v8::Value> EmitMessage(const v8::Arguments &arguments);
	static v8::Handle<v8::Value> BootstrapMock(const v8::Arguments &arguments);
};

SecureWorkerInternal::SecureWorkerInternal(const char *file_name) : enclave_id(0) {
	{
		sgx_launch_token_t launch_token;
		int launch_token_updated;
		const sgx_status_t status = sgx_create_enclave(file_name, SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &enclave_id, NULL);
		if (status != SGX_SUCCESS) throw sgx_error(status, "sgx_create_enclave");
	}
	std::cerr << "created enclave " << enclave_id << std::endl; // %%%
}

SecureWorkerInternal::~SecureWorkerInternal() {
	{
		const sgx_status_t status = sgx_destroy_enclave(enclave_id);
		if (status != SGX_SUCCESS) throw sgx_error(status, "sgx_destroy_enclave");
	}
	std::cerr << "destroyed enclave " << enclave_id << std::endl; // %%%
	enclave_id = 0;
}

void SecureWorkerInternal::init(const char *key) {
	{
		const sgx_status_t status = duk_enclave_init(enclave_id, key);
		if (status != SGX_SUCCESS) throw sgx_error(status, "duk_enclave_init");
	}
}

void SecureWorkerInternal::close() {
	{
		const sgx_status_t status = duk_enclave_close(enclave_id);
		if (status != SGX_SUCCESS) throw sgx_error(status, "duk_enclave_close");
	}
}

void SecureWorkerInternal::emitMessage(const char *message) {
	{
		const sgx_status_t status = duk_enclave_emit_message(enclave_id, message);
		if (status != SGX_SUCCESS) throw sgx_error(status, "duk_enclave_emit_message");
	}
}

void SecureWorkerInternal::bootstrapMock(sgx_sealed_data_t *out, size_t out_size,
                                         const uint8_t *additional_data, size_t additional_data_size,
                                         const uint8_t *clear, size_t data_size) {
	{
		const sgx_status_t status = duk_enclave_bootstrap_mock(enclave_id, out, out_size, additional_data, additional_data_size, clear, data_size);
		if (status != SGX_SUCCESS) throw sgx_error(status, "duk_enclave_bootstrap_mock");
	}
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
	} catch (sgx_error error) {
		error.rethrow();
		return scope.Close(v8::Undefined());
	}

	secure_worker_internal->Wrap(arguments.This());
	// Why doesn't this need scope.Close?
	return arguments.This();
}

v8::Handle<v8::Value> SecureWorkerInternal::Init(const v8::Arguments &arguments) {
	v8::HandleScope scope;

	if (!arguments[0]->IsString()) {
		v8::ThrowException(v8::Exception::TypeError(v8::String::New("Argument error")));
		return scope.Close(v8::Undefined());
	}
	v8::String::Utf8Value arg0_utf8(arguments[0]);
	SecureWorkerInternal *secure_worker_internal = node::ObjectWrap::Unwrap<SecureWorkerInternal>(arguments.This());
	try {
		entry_info entry(arguments.This());
		secure_worker_internal->init(*arg0_utf8);
	} catch (sgx_error error) {
		error.rethrow();
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
	} catch (sgx_error error) {
		error.rethrow();
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
	} catch (sgx_error error) {
		error.rethrow();
		return scope.Close(v8::Undefined());
	}
	return scope.Close(v8::Undefined());
}

v8::Handle<v8::Value> SecureWorkerInternal::BootstrapMock(const v8::Arguments &arguments) {
	v8::HandleScope scope;
	SecureWorkerInternal *secure_worker_internal = node::ObjectWrap::Unwrap<SecureWorkerInternal>(arguments.This());
	v8::Local<v8::Object> typed_array_bindings = v8::Object::New();
	v8_typed_array::AttachBindings(typed_array_bindings);

	size_t additional_data_size;
	const void *additional_data;
	if (arguments[0]->IsObject() && arguments[0].As<v8::Object>()->HasIndexedPropertiesInExternalArrayData()) {
		additional_data_size = arguments[0].As<v8::Object>()->GetIndexedPropertiesExternalArrayDataLength() * v8_typed_array::SizeOfArrayElementForType(arguments[0].As<v8::Object>()->GetIndexedPropertiesExternalArrayDataType());
		additional_data = arguments[0].As<v8::Object>()->GetIndexedPropertiesExternalArrayData();
	} else {
		additional_data_size = 0;
		additional_data = nullptr;
	}

	size_t data_size;
	const void *clear;
	if (arguments[1]->IsObject() && arguments[1].As<v8::Object>()->HasIndexedPropertiesInExternalArrayData()) {
		data_size = arguments[1].As<v8::Object>()->GetIndexedPropertiesExternalArrayDataLength() * v8_typed_array::SizeOfArrayElementForType(arguments[1].As<v8::Object>()->GetIndexedPropertiesExternalArrayDataType());
		clear = arguments[1].As<v8::Object>()->GetIndexedPropertiesExternalArrayData();
	} else {
		data_size = 0;
		clear = nullptr;
	}

	const size_t out_size = sgx_calc_sealed_data_size(additional_data_size, data_size);
	v8::Local<v8::Value> arraybuffer_arguments[] = {v8::Integer::New(out_size)};
	v8::Local<v8::Object> out_buffer = typed_array_bindings->Get(v8::String::NewSymbol("ArrayBuffer")).As<v8::Function>()->NewInstance(1, arraybuffer_arguments);
	if (out_buffer->GetIndexedPropertiesExternalArrayDataType() != v8::kExternalUnsignedByteArray || static_cast<size_t>(out_buffer->GetIndexedPropertiesExternalArrayDataLength()) != out_size) {
		v8::ThrowException(v8::Exception::TypeError(v8::String::New("out")));
		return scope.Close(v8::Undefined());
	}
	void *out = out_buffer->GetIndexedPropertiesExternalArrayData();

	try {
		entry_info entry(arguments.This());
		secure_worker_internal->bootstrapMock(
			reinterpret_cast<sgx_sealed_data_t *>(out), out_size,
			reinterpret_cast<const uint8_t *>(additional_data), additional_data_size,
			reinterpret_cast<const uint8_t *>(clear), data_size
		);
	} catch (sgx_error error) {
		error.rethrow();
		return scope.Close(v8::Undefined());
	}
	return scope.Close(out_buffer);
}

static void secureworker_internal_init(v8::Handle<v8::Object> exports, v8::Handle<v8::Object> module) {
	v8::Local<v8::FunctionTemplate> function_template = v8::FunctionTemplate::New(SecureWorkerInternal::New);
	function_template->SetClassName(v8::String::NewSymbol("SecureWorkerInternal"));
	function_template->InstanceTemplate()->SetInternalFieldCount(1);
	function_template->PrototypeTemplate()->Set(v8::String::NewSymbol("init"), v8::FunctionTemplate::New(SecureWorkerInternal::Init)->GetFunction());
	function_template->PrototypeTemplate()->Set(v8::String::NewSymbol("close"), v8::FunctionTemplate::New(SecureWorkerInternal::Close)->GetFunction());
	function_template->PrototypeTemplate()->Set(v8::String::NewSymbol("emitMessage"), v8::FunctionTemplate::New(SecureWorkerInternal::EmitMessage)->GetFunction());
	function_template->PrototypeTemplate()->Set(v8::String::NewSymbol("bootstrapMock"), v8::FunctionTemplate::New(SecureWorkerInternal::BootstrapMock)->GetFunction());
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

void duk_enclave_debug(const char *message) {
	std::cerr << message << std::endl;
}

NODE_MODULE(secureworker_internal, secureworker_internal_init);
