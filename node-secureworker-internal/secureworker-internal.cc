#include <nan.h>

#include <iomanip>
#include <iostream> // %%%
#include <sstream>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

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

void arrayBufferData(v8::Local<v8::Value> input, uint8_t **data, size_t *size) {
  v8::Local<v8::ArrayBuffer> buffer = input.As<v8::ArrayBuffer>();
  v8::Local<v8::Uint8Array> array = v8::Uint8Array::New(buffer, 0, buffer->ByteLength());
  Nan::TypedArrayContents<uint8_t> contents(array);

  *data = *contents;
  *size = buffer->ByteLength();
}

// The rest of the stuff, which is per-instance

class SecureWorkerInternal : public Nan::ObjectWrap {
private:
  bool destroyed;
public:
  sgx_enclave_id_t enclave_id;

  explicit SecureWorkerInternal(const char *file_name);
  ~SecureWorkerInternal();

  void init(const char *key);
  void destroy();
  void close();
  void emitMessage(const char *message);
  void bootstrapMock(sgx_sealed_data_t *out, size_t out_size,
                     const uint8_t *additional_data, size_t additional_data_size,
                     const uint8_t *data, size_t data_size);

  static void initQuote(sgx_target_info_t *target_info, sgx_epid_group_id_t *gid);
  static void getQuoteSize(const uint8_t *sig_rl, uint32_t sig_rl_size, uint32_t *quote_size);
  static void getQuote(const sgx_report_t *report, sgx_quote_sign_type_t quote_type, const sgx_spid_t *spid,
                       const uint8_t *sig_rl, uint32_t sig_rl_size, sgx_quote_t *quote, uint32_t quote_size);

  static Nan::Persistent<v8::Function> constructor;
  static NAN_METHOD(New);
  static NAN_METHOD(Init);
  static NAN_METHOD(Close);
  static NAN_METHOD(EmitMessage);
  static NAN_METHOD(InitQuote);
  static NAN_METHOD(GetQuote);
  static NAN_METHOD(BootstrapMock);
  static NAN_METHOD(Destroy);
};

SecureWorkerInternal::SecureWorkerInternal(const char *file_name) : destroyed(false), enclave_id(0) {
  {
    sgx_launch_token_t launch_token;
    memset(&launch_token, 0, sizeof(launch_token));
    int launch_token_updated = 0;
    const sgx_status_t status = sgx_create_enclave(file_name, SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &enclave_id, NULL);
    if (status != SGX_SUCCESS) throw sgx_error(status, "sgx_create_enclave");
  }
  std::cerr << "created enclave " << enclave_id << std::endl; // %%%
}

SecureWorkerInternal::~SecureWorkerInternal() {
  {
    if (!destroyed)
    {
      sgx_destroy_enclave(enclave_id);  
    }
  }
  std::cerr << "destroyed enclave " << enclave_id << std::endl; // %%%
  enclave_id = 0;
}

void SecureWorkerInternal::destroy() {
  destroyed = true;
  const sgx_status_t status = sgx_destroy_enclave(enclave_id);
  if (status != SGX_SUCCESS) throw sgx_error(status, "sgx_destroy_enclave");
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

void SecureWorkerInternal::initQuote(sgx_target_info_t *target_info, sgx_epid_group_id_t *gid) {
  {
    // We have to zero out buffers first. See: https://github.com/01org/linux-sgx/issues/82
    memset(target_info, 0, sizeof(*target_info));
    memset(gid, 0, sizeof(*gid));
    const sgx_status_t status = sgx_init_quote(target_info, gid);
    if (status != SGX_SUCCESS) throw sgx_error(status, "sgx_init_quote");
  }
}

void SecureWorkerInternal::getQuoteSize(const uint8_t *sig_rl, uint32_t sig_rl_size, uint32_t *quote_size) {
  {
    const sgx_status_t status = sgx_calc_quote_size(sig_rl, sig_rl_size, quote_size);
    if (status != SGX_SUCCESS) throw sgx_error(status, "sgx_get_quote_size");
  }
}

void SecureWorkerInternal::getQuote(const sgx_report_t *report, sgx_quote_sign_type_t quote_type, const sgx_spid_t *spid,
                                    const uint8_t *sig_rl, uint32_t sig_rl_size, sgx_quote_t *quote, uint32_t quote_size) {
  {
    sgx_quote_nonce_t nonce;
    sgx_report_t qe_report;

    int rc = RAND_bytes(nonce.rand, sizeof(nonce.rand));
    if (rc != 1) throw sgx_error(SGX_ERROR_UNEXPECTED, "sgx_get_quote nonce generation");

    memset(&qe_report, 0, sizeof(qe_report));
    memset(quote, 0, quote_size);

    const sgx_status_t status = sgx_get_quote(report, quote_type, spid, &nonce, sig_rl, sig_rl_size, &qe_report, quote, quote_size);
    if (status != SGX_SUCCESS) throw sgx_error(status, "sgx_get_quote");

    SHA256_CTX context;
    if(!SHA256_Init(&context)) throw sgx_error(SGX_ERROR_UNEXPECTED, "SHA256_Init");

    if(!SHA256_Update(&context, &nonce, sizeof(nonce))) throw sgx_error(SGX_ERROR_UNEXPECTED, "SHA256_Update nonce");

    if(!SHA256_Update(&context, quote, quote_size)) throw sgx_error(SGX_ERROR_UNEXPECTED, "SHA256_Update quote");

    unsigned char digest[sizeof(sgx_report_data_t)] = {0};

    if(!SHA256_Final(digest, &context)) throw sgx_error(SGX_ERROR_UNEXPECTED, "SHA256_Final");

    // Report data from qe_report should be equal to sha256(nonce || quote).
    if(memcmp(&qe_report.body.report_data, digest, sizeof(sgx_report_data_t)) != 0) throw sgx_error(SGX_ERROR_UNEXPECTED, "sgx_get_quote nonce mismatch");;
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

  uint8_t *additional_data;
  size_t additional_data_size;
  if (info[0]->IsArrayBuffer()) {
    arrayBufferData(info[0], &additional_data, &additional_data_size);
  } else {
    additional_data = nullptr;
    additional_data_size = 0;
  }

  uint8_t *data;
  size_t data_size;
  if (info[1]->IsArrayBuffer()) {
    arrayBufferData(info[1], &data, &data_size);
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

NAN_METHOD(SecureWorkerInternal::Destroy) {
  SecureWorkerInternal *secure_worker_internal = Nan::ObjectWrap::Unwrap<SecureWorkerInternal>(info.This());
  try {
    entry_info entry(info.This());
    secure_worker_internal->destroy();
  } catch (sgx_error error) {
    return error.rethrow();
  }
}

NAN_METHOD(SecureWorkerInternal::InitQuote) {
  sgx_target_info_t target_info;
  sgx_epid_group_id_t gid;

  try {
    SecureWorkerInternal::initQuote(&target_info, &gid);
  } catch (sgx_error error) {
    return error.rethrow();
  }

  v8::Local<v8::ArrayBuffer> target_info_buffer = v8::ArrayBuffer::New(v8::Isolate::GetCurrent(), sizeof(target_info));
  v8::Local<v8::ArrayBuffer> gid_buffer = v8::ArrayBuffer::New(v8::Isolate::GetCurrent(), sizeof(gid));

  v8::Local<v8::Uint8Array> target_info_array = v8::Uint8Array::New(target_info_buffer, 0, sizeof(target_info));
  v8::Local<v8::Uint8Array> gid_array = v8::Uint8Array::New(gid_buffer, 0, sizeof(gid));

  Nan::TypedArrayContents<uint8_t> target_info_uint8(target_info_array);
  Nan::TypedArrayContents<uint8_t> gid_uint8(gid_array);

  memcpy(*target_info_uint8, &target_info, sizeof(target_info));
  memcpy(*gid_uint8, &gid, sizeof(gid));

  v8::Local<v8::Object> result = Nan::New<v8::Object>();

  Nan::Set(result, Nan::New("targetInfo").ToLocalChecked(), target_info_buffer);
  Nan::Set(result, Nan::New("gid").ToLocalChecked(), gid_buffer);

  info.GetReturnValue().Set(result);
}

NAN_METHOD(SecureWorkerInternal::GetQuote) {
  if (!info[0]->IsArrayBuffer()) {
    return Nan::ThrowTypeError("Argument 0 error");
  }

  sgx_report_t *report;
  size_t report_size;
  arrayBufferData(info[0], reinterpret_cast<uint8_t **>(&report), &report_size);
  if (report_size != sizeof(sgx_report_t)) {
    return Nan::ThrowTypeError("Argument 0 error");
  }

  if (!info[1]->IsBoolean()) {
    return Nan::ThrowTypeError("Argument 1 error");
  }

  sgx_quote_sign_type_t quote_type = SGX_UNLINKABLE_SIGNATURE;
  if (Nan::To<bool>(info[1]).FromJust()) {
    quote_type = SGX_LINKABLE_SIGNATURE;
  }

  if (!info[2]->IsArrayBuffer()) {
    return Nan::ThrowTypeError("Argument 2 error");
  }

  sgx_spid_t *spid;
  size_t spid_size;
  arrayBufferData(info[2], reinterpret_cast<uint8_t **>(&spid), &spid_size);
  if (spid_size != sizeof(sgx_spid_t)) {
    return Nan::ThrowTypeError("Argument 2 error");
  }

  uint8_t *sig_rl;
  size_t sig_rl_size;
  if (info[3]->IsArrayBuffer()) {
    arrayBufferData(info[3], &sig_rl, &sig_rl_size);
  } else {
    sig_rl = nullptr;
    sig_rl_size = 0;
  }

  uint32_t quote_size;

  try {
    SecureWorkerInternal::getQuoteSize(sig_rl, sig_rl_size, &quote_size);
  } catch (sgx_error error) {
    return error.rethrow();
  }

  v8::Local<v8::ArrayBuffer> quote_buffer = v8::ArrayBuffer::New(v8::Isolate::GetCurrent(), quote_size);
  v8::Local<v8::Uint8Array> quote_array = v8::Uint8Array::New(quote_buffer, 0, quote_size);

  Nan::TypedArrayContents<uint8_t> quote_uint8(quote_array);

  sgx_quote_t *quote = reinterpret_cast<sgx_quote_t *>(*quote_uint8);

  try {
    SecureWorkerInternal::getQuote(report, quote_type, spid, sig_rl, sig_rl_size, quote, quote_size);
  } catch (sgx_error error) {
    return error.rethrow();
  }

  info.GetReturnValue().Set(quote_buffer);
}

static void secureworker_internal_init(v8::Local<v8::Object> exports, v8::Local<v8::Object> module) {
  v8::Local<v8::FunctionTemplate> function_template = Nan::New<v8::FunctionTemplate>(SecureWorkerInternal::New);

  function_template->SetClassName(Nan::New("SecureWorkerInternal").ToLocalChecked());
  function_template->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(function_template, "init", SecureWorkerInternal::Init);
  Nan::SetPrototypeMethod(function_template, "close", SecureWorkerInternal::Close);
  Nan::SetPrototypeMethod(function_template, "emitMessage", SecureWorkerInternal::EmitMessage);
  Nan::SetPrototypeMethod(function_template, "bootstrapMock", SecureWorkerInternal::BootstrapMock);
  Nan::SetPrototypeMethod(function_template, "destroy", SecureWorkerInternal::Destroy);

  Nan::SetPrototypeTemplate(function_template, "handlePostMessage", Nan::Null());

  Nan::SetMethod(function_template, "initQuote", SecureWorkerInternal::InitQuote);
  Nan::SetMethod(function_template, "getQuote", SecureWorkerInternal::GetQuote);


  SecureWorkerInternal::constructor.Reset(Nan::GetFunction(function_template).ToLocalChecked());

  Nan::Set(module, Nan::New("exports").ToLocalChecked(), Nan::GetFunction(function_template).ToLocalChecked());
}

void duk_enclave_post_message(const char *message) {
  assert(thread_entry != nullptr);
  v8::Local<v8::Value> handle_post_message = Nan::Get(thread_entry->entrant, Nan::New("handlePostMessage").ToLocalChecked()).ToLocalChecked();
  if (!handle_post_message->IsFunction()) return;
  v8::Local<v8::Value> arguments[] = {Nan::New<v8::String>(message).ToLocalChecked()};
  handle_post_message.As<v8::Function>()->Call(v8::Isolate::GetCurrent()->GetCurrentContext(), thread_entry->entrant, 1, arguments).FromMaybe(v8::Local<v8::Value>());
}

sgx_status_t duk_enclave_init_quote(sgx_target_info_t *target_info, sgx_epid_group_id_t *gid) {
  return sgx_init_quote(target_info, gid);
};

void duk_enclave_debug(const char *message) {
  std::cerr << message << std::endl;
}

NODE_MODULE(secureworker_internal, secureworker_internal_init);
