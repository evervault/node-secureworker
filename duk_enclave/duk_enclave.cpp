#include "duk_enclave_t.h"

#ifdef _WIN32
#include "sgx_debug.h"
#endif
#include "sgx_tae_service.h"
#include "sgx_tcrypto.h"
#include "sgx_trts.h"
#include "sgx_tseal.h"
#include "sgx_utils.h"
#include "duktape.h"

#include "scripts.h"

static void *get_buffer_data_notnull(duk_context *ctx, duk_idx_t index, duk_size_t *out_size) {
	void * const out = duk_get_buffer_data(ctx, index, out_size);
	if (out == NULL) {
		return reinterpret_cast<void *>(get_buffer_data_notnull);
	} else {
		return out;
	}
}

static const duk_enclave_script_t *look_up_script(const char *key) {
	for (size_t i = 0; i < MAX_SCRIPT; i++) {
		if (!strcmp(key, SCRIPTS[i].key)) return &SCRIPTS[i];
	}
	return NULL;
}

static duk_int_t peval_lstring_filename(duk_context *ctx, const char *src, duk_size_t len) {
	{
		const duk_int_t result = duk_pcompile_lstring_filename(ctx, DUK_COMPILE_EVAL, src, len);
		if (result != DUK_EXEC_SUCCESS) return result;
	}
	duk_push_global_object(ctx);
	{
		const duk_int_t result = duk_pcall_method(ctx, 0);
		if (result != DUK_EXEC_SUCCESS) return result;
	}
	return DUK_EXEC_SUCCESS;
}

static void throw_sgx_status(duk_context *ctx, sgx_status_t status, const char *source) {
	duk_push_error_object(ctx, DUK_ERR_ERROR, "%s failed (0x%04x)", source, status);
	duk_throw(ctx);
}

static void output_debug_line(const char *line) {
#ifndef NDEBUG
	duk_enclave_debug(line);
#endif
#ifdef _WIN32
	OutputDebugString(const_cast<char *>(line));
	OutputDebugString("\n");
#endif
}

static void report_error(duk_context *ctx) {
	if (duk_is_error(ctx, -1)) {
		duk_get_prop_string(ctx, -1, "stack");
		const char * const message = duk_safe_to_string(ctx, -1);
		output_debug_line(message);
		duk_pop(ctx);
	} else {
		const char * const message = duk_safe_to_string(ctx, -1);
		output_debug_line(message);
	}
}

static void report_fatal(duk_context *ctx, duk_errcode_t code, const char *msg) {
	output_debug_line(msg);
	abort();
}

struct ecc_auto_state_handle {
	duk_context *ctx;
	sgx_ecc_state_handle_t ecc_handle;
	ecc_auto_state_handle(duk_context *ctx) : ctx(ctx) {
		const sgx_status_t status = sgx_ecc256_open_context(&ecc_handle);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_ecc256_open_context");
	}
	~ecc_auto_state_handle() {
		const sgx_status_t status = sgx_ecc256_close_context(ecc_handle);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_ecc256_close_context");
	}
};

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
	const char * const key = duk_get_string(ctx, 0);
	if (key == NULL) return DUK_RET_TYPE_ERROR;
	const duk_enclave_script_t *script = look_up_script(key);
	if (script == NULL) return DUK_RET_ERROR;
	duk_push_string(ctx, script->key);
	const duk_int_t result = peval_lstring_filename(ctx, script->start, script->end - script->start);
	if (result != DUK_EXEC_SUCCESS) duk_throw(ctx);
	return 0;
}

static duk_ret_t native_debug(duk_context *ctx) {
	const char * const arg = duk_safe_to_string(ctx, 0);
	output_debug_line(arg);
	return 0;
}

static duk_ret_t native_post_quote(duk_context *ctx) {
	duk_size_t report_data_size;
	const void * const report_data = duk_get_buffer_data(ctx, 0, &report_data_size);
	if (report_data_size != sizeof(sgx_report_data_t)) return DUK_RET_ERROR;
	sgx_target_info_t qe_target_info;
	{
		const sgx_status_t status = duk_enclave_init_quote(&qe_target_info);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "duk_enclave_init_quote");
	}
	sgx_report_t report;
	{
		const sgx_status_t status = sgx_create_report(
			&qe_target_info,
			reinterpret_cast<const sgx_report_data_t *>(report_data),
			&report
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_create_report");
	}
	{
		const sgx_status_t status = duk_enclave_post_quote_report(&report);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "duk_enclave_post_quote_report");
	}
	return 0;
}

static duk_ret_t native_seal_data(duk_context *ctx) {
	duk_size_t additional_data_size;
	const void * const additional_data = duk_get_buffer_data(ctx, 0, &additional_data_size);
	duk_size_t data_size;
	const void * const data = duk_get_buffer_data(ctx, 1, &data_size);
	const uint32_t sealed_data_size = sgx_calc_sealed_data_size(additional_data_size, data_size);
	void * const sealed_data = duk_push_fixed_buffer(ctx, sealed_data_size);
	{
		const sgx_status_t status = sgx_seal_data(
			additional_data_size, reinterpret_cast<const uint8_t *>(additional_data),
			data_size, reinterpret_cast<const uint8_t *>(data),
			sealed_data_size, reinterpret_cast<sgx_sealed_data_t *>(sealed_data)
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_seal_data");
	}
	duk_push_buffer_object(ctx, -1, 0, sealed_data_size, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t native_unseal_data(duk_context *ctx) {
	duk_size_t sealed_data_size;
	const void * const sealed_data = duk_get_buffer_data(ctx, 0, &sealed_data_size);
	uint32_t additional_data_size = sgx_get_add_mac_txt_len(reinterpret_cast<const sgx_sealed_data_t *>(sealed_data));
	if (additional_data_size == UINT32_MAX) return DUK_RET_ERROR;
	uint32_t data_size = sgx_get_encrypt_txt_len(reinterpret_cast<const sgx_sealed_data_t *>(sealed_data));
	if (data_size == UINT32_MAX) return DUK_RET_ERROR;
	void * const additional_data = duk_push_fixed_buffer(ctx, additional_data_size);
	void * const data = duk_push_fixed_buffer(ctx, data_size);
	{
		const sgx_status_t status = sgx_unseal_data(
			reinterpret_cast<const sgx_sealed_data_t *>(sealed_data),
			reinterpret_cast<uint8_t *>(additional_data), &additional_data_size,
			reinterpret_cast<uint8_t *>(data), &data_size
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_seal_data");
	}
	duk_push_object(ctx);
	// [additional_data plain buffer] [data plain buffer] [object]
	duk_push_buffer_object(ctx, -3, 0, additional_data_size, DUK_BUFOBJ_ARRAYBUFFER);
	duk_put_prop_string(ctx, -2, "additionalData");
	duk_push_buffer_object(ctx, -2, 0, data_size, DUK_BUFOBJ_ARRAYBUFFER);
	duk_put_prop_string(ctx, -2, "data");
	return 1;
}

static duk_ret_t native_get_time(duk_context *ctx) {
	// No 64-bit integers. Thanks, JavaScript.
	void * const current_time = duk_push_fixed_buffer(ctx, sizeof(sgx_time_t));
	void * const time_source_nonce = duk_push_fixed_buffer(ctx, sizeof(sgx_time_source_nonce_t));
	{
		const sgx_status_t status = sgx_get_trusted_time(
			reinterpret_cast<sgx_time_t *>(current_time),
			reinterpret_cast<sgx_time_source_nonce_t *>(time_source_nonce)
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_get_trusted_time");
	}
	duk_push_object(ctx);
	// [time plain buffer] [time_source_nonce plain buffer] [object]
	duk_push_buffer_object(ctx, -3, 0, sizeof(sgx_time_t), DUK_BUFOBJ_ARRAYBUFFER);
	duk_put_prop_string(ctx, -2, "currentTime");
	duk_push_buffer_object(ctx, -2, 0, sizeof(sgx_time_source_nonce_t), DUK_BUFOBJ_ARRAYBUFFER);
	duk_put_prop_string(ctx, -2, "timeSourceNonce");
	return 1;
}

static duk_ret_t native_create_monotonic_counter(duk_context *ctx) {
	void * const uuid = duk_push_fixed_buffer(ctx, sizeof(sgx_mc_uuid_t));
	uint32_t value;
	{
		const sgx_status_t status = sgx_create_monotonic_counter(reinterpret_cast<sgx_mc_uuid_t *>(uuid), &value);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_create_monotonic_counter");
	}
	duk_push_object(ctx);
	// [uuid plain buffer] [object]
	duk_push_buffer_object(ctx, -2, 0, sizeof(sgx_mc_uuid_t), DUK_BUFOBJ_ARRAYBUFFER);
	duk_put_prop_string(ctx, -2, "uuid");
	duk_push_number(ctx, value);
	duk_put_prop_string(ctx, -2, "value");
	return 1;
}

static duk_ret_t native_destroy_monotonic_counter(duk_context *ctx) {
	duk_size_t uuid_size;
	const void * const uuid = duk_get_buffer_data(ctx, 0, &uuid_size);
	if (uuid_size != sizeof(sgx_mc_uuid_t)) return DUK_RET_ERROR;
	{
		const sgx_status_t status = sgx_destroy_monotonic_counter(
			reinterpret_cast<const sgx_mc_uuid_t *>(uuid)
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_destroy_monotonic_counter");
	}
	return 0;
}

static duk_ret_t native_increment_monotonic_counter(duk_context *ctx) {
	duk_size_t uuid_size;
	const void * const uuid = duk_get_buffer_data(ctx, 0, &uuid_size);
	uint32_t value;
	if (uuid_size != sizeof(sgx_mc_uuid_t)) return DUK_RET_ERROR;
	{
		const sgx_status_t status = sgx_increment_monotonic_counter(
			reinterpret_cast<const sgx_mc_uuid_t *>(uuid),
			&value
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_increment_monotonic_counter");
	}
	duk_push_number(ctx, value);
	return 1;
}

static duk_ret_t native_read_monotonic_counter(duk_context *ctx) {
	duk_size_t uuid_size;
	const void * const uuid = duk_get_buffer_data(ctx, 0, &uuid_size);
	uint32_t value;
	if (uuid_size != sizeof(sgx_mc_uuid_t)) return DUK_RET_ERROR;
	{
		const sgx_status_t status = sgx_read_monotonic_counter(
			reinterpret_cast<const sgx_mc_uuid_t *>(uuid),
			&value
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_read_monotonic_counter");
	}
	duk_push_number(ctx, value);
	return 1;
}

static duk_ret_t native_get_random_values(duk_context *ctx) {
	duk_size_t array_size;
	void * const array = duk_get_buffer_data(ctx, 0, &array_size);
	{
		const sgx_status_t status = sgx_read_rand(
			reinterpret_cast<unsigned char *>(array), array_size
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_read_rand");
	}
	return 1;
}

// AES-256.

static duk_ret_t native_sha256_digest(duk_context *ctx) {
	duk_size_t data_size;
	const void * const data = get_buffer_data_notnull(ctx, 0, &data_size);
	void * const out = duk_push_fixed_buffer(ctx, sizeof(sgx_sha256_hash_t));
	{
		const sgx_status_t status = sgx_sha256_msg(
			reinterpret_cast<const uint8_t *>(data), data_size,
			reinterpret_cast<sgx_sha256_hash_t *>(out)
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_sha256_msg");
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
	const void * const data = get_buffer_data_notnull(ctx, 3, &data_size);
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
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_rijndael128GCM_encrypt");
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
	const void * const data = get_buffer_data_notnull(ctx, 3, &data_size);
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
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_rijndael128GCM_decrypt");
	}
	duk_push_buffer_object(ctx, -1, 0, out_size, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

// AES-CMAC. only 128-bit keys, only 128-bit MACs

static duk_ret_t native_aescmac_sign(duk_context *ctx) {
	duk_size_t key_size;
	void * const key = duk_get_buffer_data(ctx, 0, &key_size);
	duk_size_t data_size;
	const void * const data = get_buffer_data_notnull(ctx, 1, &data_size);
	if (key_size != sizeof(sgx_cmac_128bit_key_t)) return DUK_RET_ERROR;
	void * const out = duk_push_fixed_buffer(ctx, sizeof(sgx_cmac_128bit_tag_t));
	{
		const sgx_status_t status = sgx_rijndael128_cmac_msg(
			reinterpret_cast<sgx_cmac_128bit_key_t *>(key),
			reinterpret_cast<const uint8_t *>(data), data_size,
			reinterpret_cast<sgx_cmac_128bit_tag_t *>(out)
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_rijndael128_cmac_msg");
	}
	duk_push_buffer_object(ctx, -1, 0, sizeof(sgx_cmac_128bit_tag_t), DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

// AES-CTR. only 128-bit keys

static duk_ret_t native_aesctr_encrypt(duk_context *ctx) {
	duk_size_t counter_size;
	const void * const counter = duk_get_buffer_data(ctx, 0, &counter_size);
	duk_small_int_t length = duk_get_int(ctx, 1);
	duk_size_t key_size;
	const void * const key = duk_get_buffer_data(ctx, 2, &key_size);
	duk_size_t data_size;
	const void * const data = get_buffer_data_notnull(ctx, 3, &data_size);
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
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_aes_ctr_encrypt");
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
	const void * const data = get_buffer_data_notnull(ctx, 3, &data_size);
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
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_aes_ctr_decrypt");
	}
	duk_push_buffer_object(ctx, -1, 0, data_size, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

// EC. only P-256

static duk_ret_t native_ec_generate_key(duk_context *ctx) {
	void * const public_key = duk_push_fixed_buffer(ctx, sizeof(sgx_ec256_public_t));
	void * const private_key = duk_push_fixed_buffer(ctx, sizeof(sgx_ec256_private_t));
	{
		ecc_auto_state_handle ecc_context(ctx);
		const sgx_status_t status = sgx_ecc256_create_key_pair(
			reinterpret_cast<sgx_ec256_private_t *>(private_key),
			reinterpret_cast<sgx_ec256_public_t *>(public_key),
			ecc_context.ecc_handle
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_ecc256_create_key_pair");
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
	{
		ecc_auto_state_handle ecc_context(ctx);
		const sgx_status_t status = sgx_ecc256_compute_shared_dhkey(
			reinterpret_cast<sgx_ec256_private_t *>(private_key),
			reinterpret_cast<sgx_ec256_public_t *>(public_key),
			reinterpret_cast<sgx_ec256_dh_shared_t *>(out),
			ecc_context.ecc_handle
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_ecc256_compute_shared_dhkey");
	}
	duk_push_buffer_object(ctx, -1, 0, sizeof(sgx_ec256_dh_shared_t), DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

// ECDSA. only P-256, only SHA256

static duk_ret_t native_ecdsa_sign(duk_context *ctx) {
	duk_size_t key_size;
	void * const key = duk_get_buffer_data(ctx, 0, &key_size);
	duk_size_t data_size;
	const void * const data = get_buffer_data_notnull(ctx, 1, &data_size);
	if (key_size != sizeof(sgx_ec256_private_t)) return DUK_RET_ERROR;
	void * const out = duk_push_fixed_buffer(ctx, sizeof(sgx_ec256_signature_t));
	{
		ecc_auto_state_handle ecc_context(ctx);
		const sgx_status_t status = sgx_ecdsa_sign(
			reinterpret_cast<const uint8_t *>(data), data_size,
			reinterpret_cast<sgx_ec256_private_t *>(key),
			reinterpret_cast<sgx_ec256_signature_t *>(out),
			ecc_context.ecc_handle
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_ecdsa_sign");
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
	const void * const data = get_buffer_data_notnull(ctx, 2, &data_size);
	if (key_size != sizeof(sgx_ec256_public_t) || signature_size != sizeof(sgx_ec256_signature_t)) return DUK_RET_ERROR;
	uint8_t out;
	{
		ecc_auto_state_handle ecc_context(ctx);
		const sgx_status_t status = sgx_ecdsa_verify(
			reinterpret_cast<const uint8_t *>(data), data_size,
			reinterpret_cast<sgx_ec256_public_t *>(key),
			reinterpret_cast<sgx_ec256_signature_t *>(signature),
			&out,
			ecc_context.ecc_handle
		);
		if (status != SGX_SUCCESS) throw_sgx_status(ctx, status, "sgx_ecdsa_verify");
	}
	duk_push_boolean(ctx, out == SGX_EC_VALID);
	return 1;
}

//

static const duk_function_list_entry native_methods[] = {
	{"postMessage", native_post_message, 1},
	{"nextTick", native_next_tick, 1},
	{"importScript", native_import_script, 1},
	{"debug", native_debug, 1},
	{"postQuote", native_post_quote, 1},
	{"sealData", native_seal_data, 2},
	{"unsealData", native_unseal_data, 1},
	{"getTime", native_get_time, 0},
	{"createMonotonicCounter", native_create_monotonic_counter, 0},
	{"destroyMonotonicCounter", native_destroy_monotonic_counter, 1},
	{"incrementMonotonicCounter", native_increment_monotonic_counter, 1},
	{"readMonotonicCounter", native_read_monotonic_counter, 1},
	{"getRandomValues", native_get_random_values, 1},
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
		const duk_int_t result = duk_pcall(ctx, 0);
		if (result == DUK_EXEC_ERROR) report_error(ctx);
		duk_pop(ctx);
	}
	duk_pop_3(ctx);
}

static duk_context *ctx = NULL;

void duk_enclave_init(const char *key) {
	if (ctx != NULL) abort();
	ctx = duk_create_heap(NULL, NULL, NULL, NULL, report_fatal);
	if (ctx == NULL) abort();
	{
		sgx_status_t status = sgx_create_pse_session();
		if (status != SGX_SUCCESS) abort();
	}
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
	if (key == NULL) abort();
	const duk_enclave_script_t *script = look_up_script(key);
	if (script == NULL) abort();
	duk_push_string(ctx, script->key);
	const duk_int_t result = peval_lstring_filename(ctx, script->start, script->end - script->start);
	if (result == DUK_EXEC_ERROR) report_error(ctx);
	duk_pop(ctx);
	spin_microtasks(ctx);
}

void duk_enclave_close() {
	if (ctx == NULL) abort();
	duk_destroy_heap(ctx);
	ctx = NULL;
	{
		sgx_status_t status = sgx_close_pse_session();
		if (status != SGX_SUCCESS) abort();
	}
}

void duk_enclave_emit_message(const char *message) {
	if (ctx == NULL) abort();
	duk_get_global_string(ctx, "_dukEnclaveHandlers");
	duk_get_prop_string(ctx, -1, "emitMessage");
	duk_push_string(ctx, message);
	const duk_int_t result = duk_pcall(ctx, 1);
	if (result == DUK_EXEC_ERROR) report_error(ctx);
	duk_pop(ctx);
	spin_microtasks(ctx);
}
