#include "duk_enclave_t.h"

#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include "duktape.h"

#include "scripts.h"

static duk_ret_t native_post_message(duk_context *ctx) {
	const char * const message = duk_get_string(ctx, 0);
	if (message == NULL) return DUK_RET_TYPE_ERROR;
	{
		const sgx_status_t status = duk_enclave_post_message(message);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	return 0;
}

static duk_ret_t native_next_tick(duk_context *ctx) {
	if (!duk_is_function(ctx, 0)) return DUK_RET_TYPE_ERROR;
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, "microtasks");
	duk_push_string(ctx, "push");
	// [arg0] [heap_stash] [heap_stash.microtasks] ["push"]
	duk_dup(ctx, 0);
	duk_call_prop(ctx, -3, 1);
	return 0;
}

static duk_ret_t native_import_script(duk_context *ctx) {
	if (!duk_is_number(ctx, 0)) return DUK_RET_TYPE_ERROR;
	const int key = duk_get_int(ctx, 0);
	if (key < 0 || key >= MAX_SCRIPT) return DUK_RET_RANGE_ERROR;
	duk_eval_string_noresult(ctx, SCRIPTS[key]);
	return 0;
}

static duk_ret_t native_encode_string(duk_context *ctx) {
	duk_size_t in_size;
	const char * const in = duk_get_lstring(ctx, 0, &in_size);
	if (in == NULL) return DUK_RET_TYPE_ERROR;
	duk_push_fixed_buffer(ctx, in_size);
	memcpy(duk_get_buffer_data(ctx, -1, NULL), in, in_size);
	duk_push_buffer_object(ctx, -1, 0, in_size, DUK_BUFOBJ_UINT8ARRAY);
	return 1;
}

static duk_ret_t native_decode_string(duk_context *ctx) {
	duk_size_t in_size;
	const void * const in = duk_get_buffer_data(ctx, 0, &in_size);
	if (in == NULL) return DUK_RET_TYPE_ERROR;
	duk_push_lstring(ctx, reinterpret_cast<const char *>(in), in_size);
	return 1;
}

// AES-256.

static duk_ret_t native_sha256_digest(duk_context *ctx) {
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 0, &data_size);
	void * const out = duk_push_fixed_buffer(ctx, sizeof(sgx_sha256_hash_t));
	{
		const sgx_status_t status = sgx_sha256_msg(
			reinterpret_cast<const uint8_t *>(data), data_size,
			reinterpret_cast<sgx_sha256_hash_t *>(out)
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) abort();
	}
	duk_push_buffer_object(ctx, -1, 0, sizeof(sgx_sha256_hash_t), DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

// AES-GCM. only 128-bit keys, only 12-byte IVs

static duk_ret_t native_aesgcm_encrypt(duk_context *ctx) {
	duk_size_t iv_size;
	const void * const iv = duk_get_buffer_data(ctx, 0, &iv_size);
	duk_size_t additional_data_size;
	const void * const additional_data = duk_get_buffer_data(ctx, 1, &additional_data_size);
	duk_size_t key_size;
	const void * const key = duk_get_buffer_data(ctx, 2, &key_size);
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 3, &data_size);
	if (key_size != sizeof(sgx_aes_gcm_128bit_key_t)) return DUK_RET_ERROR;
	const duk_size_t out_size = data_size + sizeof(sgx_aes_gcm_128bit_tag_t);
	void * const out = duk_push_fixed_buffer(ctx, out_size);
	void * const tag = reinterpret_cast<uint8_t *>(out) + data_size;
	{
		const sgx_status_t status = sgx_rijndael128GCM_encrypt(
			reinterpret_cast<const sgx_aes_gcm_128bit_key_t *>(key),
			reinterpret_cast<const uint8_t *>(data), data_size,
			reinterpret_cast<uint8_t *>(out),
			reinterpret_cast<const uint8_t *>(iv), iv_size,
			reinterpret_cast<const uint8_t *>(additional_data), additional_data_size,
			reinterpret_cast<sgx_aes_gcm_128bit_tag_t *>(tag)
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	duk_push_buffer_object(ctx, -1, 0, out_size, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t native_aesgcm_decrypt(duk_context *ctx) {
	duk_size_t iv_size;
	const void * const iv = duk_get_buffer_data(ctx, 0, &iv_size);
	duk_size_t additional_data_size;
	const void * const additional_data = duk_get_buffer_data(ctx, 1, &additional_data_size);
	duk_size_t key_size;
	const void * const key = duk_get_buffer_data(ctx, 2, &key_size);
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 3, &data_size);
	if (key_size != sizeof(sgx_aes_gcm_128bit_key_t) || data_size < sizeof(sgx_aes_gcm_128bit_tag_t)) return DUK_RET_ERROR;
	const duk_size_t out_size = data_size - sizeof(sgx_aes_gcm_128bit_tag_t);
	void * const out = duk_push_fixed_buffer(ctx, out_size);
	const void * const tag = reinterpret_cast<const uint8_t *>(data) + out_size;
	{
		const sgx_status_t status = sgx_rijndael128GCM_decrypt(
			reinterpret_cast<const sgx_aes_gcm_128bit_key_t *>(key),
			reinterpret_cast<const uint8_t *>(data), out_size,
			reinterpret_cast<uint8_t *>(out),
			reinterpret_cast<const uint8_t *>(iv), iv_size,
			reinterpret_cast<const uint8_t *>(additional_data), additional_data_size,
			reinterpret_cast<const sgx_aes_gcm_128bit_tag_t *>(tag)
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	duk_push_buffer_object(ctx, -1, 0, out_size, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

// AES-CMAC. only 128-bit keys, only 128-bit MACs

static duk_ret_t native_aescmac_sign(duk_context *ctx) {
	duk_size_t key_size;
	void * const key = duk_get_buffer_data(ctx, 0, &key_size);
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 1, &data_size);
	if (key_size != sizeof(sgx_cmac_128bit_key_t)) return DUK_RET_ERROR;
	void * const out = duk_push_fixed_buffer(ctx, sizeof(sgx_cmac_128bit_tag_t));
	{
		const sgx_status_t status = sgx_rijndael128_cmac_msg(
			reinterpret_cast<sgx_cmac_128bit_key_t *>(key),
			reinterpret_cast<const uint8_t *>(data), data_size,
			reinterpret_cast<sgx_cmac_128bit_tag_t *>(out)
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	duk_push_buffer_object(ctx, -1, 0, sizeof(sgx_cmac_128bit_tag_t), DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

// AES-CTR. only 128-bit keys

static duk_ret_t native_aesctr_encrypt(duk_context *ctx) {
	duk_small_int_t length = duk_get_int(ctx, 0);
	duk_size_t counter_size;
	const void * const counter = duk_get_buffer_data(ctx, 1, &counter_size);
	duk_size_t key_size;
	const void * const key = duk_get_buffer_data(ctx, 2, &key_size);
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 3, &data_size);
	if (counter_size != 16 || key_size != sizeof(sgx_aes_ctr_128bit_key_t)) return DUK_RET_ERROR;
	uint8_t counter_scratch[16];
	memcpy(counter_scratch, counter, 16);
	void * const out = duk_push_fixed_buffer(ctx, data_size);
	{
		const sgx_status_t status = sgx_aes_ctr_encrypt(
			reinterpret_cast<const sgx_aes_ctr_128bit_key_t *>(key),
			reinterpret_cast<const uint8_t *>(data), data_size,
			counter_scratch,
			length,
			reinterpret_cast<uint8_t *>(out)
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	duk_push_buffer_object(ctx, -1, 0, data_size, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t native_aesctr_decrypt(duk_context *ctx) {
	duk_small_int_t length = duk_get_int(ctx, 0);
	duk_size_t counter_size;
	const void * const counter = duk_get_buffer_data(ctx, 1, &counter_size);
	duk_size_t key_size;
	const void * const key = duk_get_buffer_data(ctx, 2, &key_size);
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 3, &data_size);
	if (counter_size != 16 || key_size != sizeof(sgx_aes_ctr_128bit_key_t)) return DUK_RET_ERROR;
	uint8_t counter_scratch[16];
	memcpy(counter_scratch, counter, 16);
	void * const out = duk_push_fixed_buffer(ctx, data_size);
	{
		const sgx_status_t status = sgx_aes_ctr_decrypt(
			reinterpret_cast<const sgx_aes_ctr_128bit_key_t *>(key),
			reinterpret_cast<const uint8_t *>(data), data_size,
			counter_scratch,
			length,
			reinterpret_cast<uint8_t *>(out)
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	duk_push_buffer_object(ctx, -1, 0, data_size, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

// EC. only P-256

static duk_ret_t native_ec_generate_key(duk_context *ctx) {
	void * const public_key = duk_push_fixed_buffer(ctx, sizeof(sgx_ec256_public_t));
	void * const private_key = duk_push_fixed_buffer(ctx, sizeof(sgx_ec256_private_t));
	sgx_ecc_state_handle_t ecc_handle;
	{
		const sgx_status_t status = sgx_ecc256_open_context(&ecc_handle);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	{
		const sgx_status_t status = sgx_ecc256_create_key_pair(
			reinterpret_cast<sgx_ec256_private_t *>(private_key),
			reinterpret_cast<sgx_ec256_public_t *>(public_key),
			ecc_handle
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	{
		const sgx_status_t status = sgx_ecc256_close_context(ecc_handle);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	duk_push_object(ctx);
	// [public plain buffer] [private plain buffer] [object]
	duk_push_buffer_object(ctx, -3, 0, sizeof(sgx_ec256_public_t), DUK_BUFOBJ_ARRAYBUFFER);
	duk_put_prop_string(ctx, -2, "publicKey");
	duk_push_buffer_object(ctx, -2, 0, sizeof(sgx_ec256_private_t), DUK_BUFOBJ_ARRAYBUFFER);
	duk_put_prop_string(ctx, -2, "privateKey");
	return 1;
}

// ECDH. only P-256

static duk_ret_t native_ecdh_derive_bits(duk_context *ctx) {
	duk_size_t public_key_size;
	void * const public_key = duk_get_buffer_data(ctx, 0, &public_key_size);
	duk_size_t private_key_size;
	void * const private_key = duk_get_buffer_data(ctx, 1, &private_key_size);
	if (public_key_size != sizeof(sgx_ec256_public_t) || private_key_size != sizeof(sgx_ec256_private_t)) return DUK_RET_ERROR;
	void * const out = duk_push_fixed_buffer(ctx, sizeof(sgx_ec256_dh_shared_t));
	sgx_ecc_state_handle_t ecc_handle;
	{
		const sgx_status_t status = sgx_ecc256_open_context(&ecc_handle);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	{
		const sgx_status_t status = sgx_ecc256_compute_shared_dhkey(
			reinterpret_cast<sgx_ec256_private_t *>(private_key),
			reinterpret_cast<sgx_ec256_public_t *>(public_key),
			reinterpret_cast<sgx_ec256_dh_shared_t *>(out),
			ecc_handle
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	{
		const sgx_status_t status = sgx_ecc256_close_context(ecc_handle);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	duk_push_buffer_object(ctx, -1, 0, sizeof(sgx_ec256_dh_shared_t), DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

// ECDSA. only P-256, only SHA256

static duk_ret_t native_ecdsa_sign(duk_context *ctx) {
	duk_size_t key_size;
	void * const key = duk_get_buffer_data(ctx, 0, &key_size);
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 1, &data_size);
	if (key_size != sizeof(sgx_ec256_private_t)) return DUK_RET_ERROR;
	void * const out = duk_push_fixed_buffer(ctx, sizeof(sgx_ec256_signature_t));
	sgx_ecc_state_handle_t ecc_handle;
	{
		const sgx_status_t status = sgx_ecc256_open_context(&ecc_handle);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	{
		const sgx_status_t status = sgx_ecdsa_sign(
			reinterpret_cast<const uint8_t *>(data), data_size,
			reinterpret_cast<sgx_ec256_private_t *>(key),
			reinterpret_cast<sgx_ec256_signature_t *>(out),
			ecc_handle
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	{
		const sgx_status_t status = sgx_ecc256_close_context(ecc_handle);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	duk_push_buffer_object(ctx, -1, 0, sizeof(sgx_ec256_signature_t), DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t native_ecdsa_verify(duk_context *ctx) {
	duk_size_t key_size;
	void * const key = duk_get_buffer_data(ctx, 0, &key_size);
	duk_size_t signature_size;
	void * const signature = duk_get_buffer_data(ctx, 1, &signature_size);
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 2, &data_size);
	if (key_size != sizeof(sgx_ec256_private_t) || signature_size != sizeof(sgx_ec256_signature_t)) return DUK_RET_ERROR;
	uint8_t out;
	sgx_ecc_state_handle_t ecc_handle;
	{
		const sgx_status_t status = sgx_ecc256_open_context(&ecc_handle);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	{
		const sgx_status_t status = sgx_ecdsa_verify(
			reinterpret_cast<const uint8_t *>(data), data_size,
			reinterpret_cast<sgx_ec256_public_t *>(key),
			reinterpret_cast<sgx_ec256_signature_t *>(signature),
			&out,
			ecc_handle
		);
		if (status == SGX_ERROR_INVALID_PARAMETER) return DUK_RET_ERROR;
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	{
		const sgx_status_t status = sgx_ecc256_close_context(ecc_handle);
		if (status != SGX_SUCCESS) return DUK_RET_INTERNAL_ERROR;
	}
	if (out == SGX_EC_VALID) {
		duk_push_true(ctx);
	} else {
		duk_push_false(ctx);
	}
	return 1;
}

//

static const duk_function_list_entry native_methods[] = {
	{"postMessage", native_post_message, 1},
	{"nextTick", native_next_tick, 1},
	{"importScript", native_import_script, 1},
	{"encodeString", native_encode_string, 1},
	{"decodeString", native_decode_string, 1},
	{"sha256Digest", native_sha256_digest, 1},
	{"aesgcmEncrypt", native_aesgcm_encrypt, 4},
	{"aesgcmDecrypt", native_aesgcm_decrypt, 4},
	{"aescmacSign", native_aescmac_sign, 2},
	{"aesctrEncrypt", native_aesctr_encrypt, 4},
	{"aesctrDecrypt", native_aesctr_decrypt, 4},
	{"ecGenerateKey", native_ec_generate_key, 0},
	{"ecdhDeriveBits", native_ecdh_derive_bits, 2},
	{"ecdsaSign", native_ecdsa_sign, 2},
	{"ecdsaVerify", native_ecdsa_verify, 3},
	{NULL, NULL, 0},
};

static void spin_microtasks(duk_context *ctx) {
	duk_push_heap_stash(ctx);
	duk_get_prop_string(ctx, -1, "microtasks");
	duk_get_prop_string(ctx, -1, "shift");
	// [heap_stash] [heap_stash.microtasks] [heap_stash.microtasks.shift]
	for (;;) {
		duk_dup(ctx, -1);
		duk_dup(ctx, -3);
		duk_call_method(ctx, 0);
		if (duk_is_undefined(ctx, -1)) break;
		duk_pcall(ctx, 0);
		duk_pop(ctx);
	}
	duk_pop_3(ctx);
}

static duk_context *ctx = NULL;

void duk_enclave_init(int key) {
	if (ctx != NULL) abort();
	if (key < 0 || key >= MAX_SCRIPT) abort();
	ctx = duk_create_heap_default();
	if (ctx == NULL) abort();
	// Framework code
	duk_push_heap_stash(ctx);
	duk_push_array(ctx);
	duk_put_prop_string(ctx, -2, "microtasks");
	duk_pop(ctx);
	duk_push_object(ctx);
	duk_put_function_list(ctx, -1, native_methods);
	duk_put_global_string(ctx, "_dukEnclaveNative");
	duk_push_object(ctx);
	duk_put_global_string(ctx, "_dukEnclaveHandlers");
	// Application code
	duk_eval_string_noresult(ctx, SCRIPTS[key]);
	spin_microtasks(ctx);
}

void duk_enclave_close() {
	if (ctx == NULL) abort();
	duk_destroy_heap(ctx);
	ctx = NULL;
}

void duk_enclave_emit_message(const char *message) {
	if (ctx == NULL) abort();
	duk_get_global_string(ctx, "_dukEnclaveHandlers");
	duk_get_prop_string(ctx, -1, "emitMessage");
	duk_push_string(ctx, message);
	duk_pcall(ctx, 1);
	duk_pop(ctx);
	spin_microtasks(ctx);
}
