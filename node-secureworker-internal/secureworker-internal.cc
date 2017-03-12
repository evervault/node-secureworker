#include <nan.h>

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
	v8::Local<v8::Object> entrant;
	entry_info(v8::Local<v8::Object> entrant) : previous(thread_entry), entrant(entrant) {
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
		Nan::ThrowError(ss.str().c_str());
	}
};

// The rest of the stuff, which is per-instance

class SecureWorkerInternal : public Nan::ObjectWrap {
public:
	sgx_enclave_id_t enclave_id;

	explicit SecureWorkerInternal(const char *file_name);
	~SecureWorkerInternal();

	void init(const char *key);
	void close();
	void emitMessage(const char *message);
	void bootstrapMock(sgx_sealed_data_t *out, size_t out_size,
	                   const uint8_t *additional_data, size_t additional_data_size,
	                   const uint8_t *data, size_t data_size);

	static Nan::Persistent<v8::Function> constructor;
	static NAN_METHOD(New);
	static NAN_METHOD(Init);
	static NAN_METHOD(Close);
	static NAN_METHOD(EmitMessage);
	static NAN_METHOD(BootstrapMock);
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
                                         const uint8_t *data, size_t data_size) {
	{
		const sgx_status_t status = duk_enclave_bootstrap_mock(enclave_id, out, out_size, additional_data, additional_data_size, data, data_size);
		if (status != SGX_SUCCESS) throw sgx_error(status, "duk_enclave_bootstrap_mock");
	}
}

Nan::Persistent<v8::Function> SecureWorkerInternal::constructor;

NAN_METHOD(SecureWorkerInternal::New) {
	if (!info.IsConstructCall()) {
		return Nan::ThrowError("SecureWorkerInternal called not as a constructor");
	}
	if (!info[0]->IsString()) {
		return Nan::ThrowTypeError("Argument error");
	}
	Nan::Utf8String arg0_utf8(info[0]);
	SecureWorkerInternal *secure_worker_internal;
	try {
		entry_info entry(info.This());
		secure_worker_internal = new SecureWorkerInternal(*arg0_utf8);
	} catch (sgx_error error) {
		return error.rethrow();
	}

	secure_worker_internal->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(SecureWorkerInternal::Init) {
	if (!info[0]->IsString()) {
		return Nan::ThrowTypeError("Argument error");
	}
	Nan::Utf8String arg0_utf8(info[0]);
	SecureWorkerInternal *secure_worker_internal = Nan::ObjectWrap::Unwrap<SecureWorkerInternal>(info.This());
	try {
		entry_info entry(info.This());
		secure_worker_internal->init(*arg0_utf8);
	} catch (sgx_error error) {
		return error.rethrow();
	}
}

NAN_METHOD(SecureWorkerInternal::Close) {
	SecureWorkerInternal *secure_worker_internal = Nan::ObjectWrap::Unwrap<SecureWorkerInternal>(info.This());
	try {
		entry_info entry(info.This());
		secure_worker_internal->close();
	} catch (sgx_error error) {
		return error.rethrow();
	}
}

NAN_METHOD(SecureWorkerInternal::EmitMessage) {
	if (!info[0]->IsString()) {
		return Nan::ThrowTypeError("Argument error");
	}
	Nan::Utf8String arg0_utf8(info[0]);
	SecureWorkerInternal *secure_worker_internal = Nan::ObjectWrap::Unwrap<SecureWorkerInternal>(info.This());
	try {
		entry_info entry(info.This());
		secure_worker_internal->emitMessage(*arg0_utf8);
	} catch (sgx_error error) {
		return error.rethrow();
	}
}

NAN_METHOD(SecureWorkerInternal::BootstrapMock) {
	SecureWorkerInternal *secure_worker_internal = Nan::ObjectWrap::Unwrap<SecureWorkerInternal>(info.This());

	size_t additional_data_size;
	const uint8_t *additional_data;
	if (info[0]->IsArrayBuffer()) {
	  Nan::TypedArrayContents<uint8_t> arg0_uint8(info[0]);
		additional_data = *arg0_uint8;
		additional_data_size = info[0].As<v8::Uint8Array>()->Length();
	} else {
		additional_data = nullptr;
		additional_data_size = 0;
	}

	size_t data_size;
	const uint8_t *data;
	if (info[1]->IsArrayBuffer()) {
	  Nan::TypedArrayContents<uint8_t> arg1_uint8(info[1]);
		data = *arg1_uint8;
		data_size = info[1].As<v8::Uint8Array>()->Length();
	} else {
		data = nullptr;
		data_size = 0;
	}

	const size_t out_size = sgx_calc_sealed_data_size(additional_data_size, data_size);
  v8::Local<v8::ArrayBuffer> out_buffer = v8::ArrayBuffer::New(v8::Isolate::GetCurrent(), out_size);
  v8::Local<v8::Uint8Array> out_array = v8::Uint8Array::New(out_buffer, 0, out_size);

  Nan::TypedArrayContents<uint8_t> out_uint8(out_array);

	sgx_sealed_data_t *out = reinterpret_cast<sgx_sealed_data_t *>(*out_uint8);

	try {
		entry_info entry(info.This());
		secure_worker_internal->bootstrapMock(
			out, out_size,
			additional_data, additional_data_size,
			data, data_size
		);
	} catch (sgx_error error) {
		return error.rethrow();
	}
	info.GetReturnValue().Set(out_buffer);
}

static void secureworker_internal_init(v8::Local<v8::Object> exports) {
	v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(SecureWorkerInternal::New);

	function_template->SetClassName(Nan::New("SecureWorkerInternal").ToLocalChecked());
	function_template->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(function_template, "init", SecureWorkerInternal::Init);
	Nan::SetPrototypeMethod(function_template, "close", SecureWorkerInternal::Close);
	Nan::SetPrototypeMethod(function_template, "emitMessage", SecureWorkerInternal::EmitMessage);
	Nan::SetPrototypeMethod(function_template, "bootstrapMock", SecureWorkerInternal::BootstrapMock);
	Nan::SetPrototypeTemplate(function_template, "handlePostMessage", Nan::Null());

  SecureWorkerInternal::constructor.Reset(Nan::GetFunction(function_template).ToLocalChecked());

  Nan::Set(exports, Nan::New("SecureWorkerInternal").ToLocalChecked(), Nan::GetFunction(function_template).ToLocalChecked());
}

void duk_enclave_post_message(const char *message) {
	assert(thread_entry != nullptr);
	v8::Local<v8::Value> handle_post_message = Nan::Get(thread_entry->entrant, Nan::New("handlePostMessage").ToLocalChecked()).ToLocalChecked();
	if (!handle_post_message->IsFunction()) return;
	v8::Local<v8::Value> arguments[] = {Nan::New<v8::String>(message).ToLocalChecked()};
	handle_post_message.As<v8::Function>()->Call(thread_entry->entrant, 1, arguments);
}

void duk_enclave_debug(const char *message) {
	std::cerr << message << std::endl;
}

NODE_MODULE(secureworker_internal, secureworker_internal_init);
