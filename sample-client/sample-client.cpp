// sample-client.cpp : Defines the entry point for the console application.
//

#include <iostream>

#include "sgx_uae_service.h"
#include "sgx_urts.h"
#include "duk_enclave_u.h"

// Test vector from RFC 4754
#define SIG_HEX "CB28E0999B9C7715FD0A80D8E47A77079716CBBF917DD72E97566EA1C066957C86FA3BB4E26CAD5BF90B7F81899256CE7594BB1EA0C89212748BFF3B3D5B0315"
#define DATA_HEX "616263"

void duk_enclave_post_message(const char *message) {
	std::cout << message << std::endl;
}

void duk_enclave_debug(const char *message) {
	std::cerr << message << std::endl;
}

void duk_enclave_init_quote(sgx_target_info_t *p_target_info) {
	{
		sgx_epid_group_id_t gid;
		const sgx_status_t status = sgx_init_quote(p_target_info, &gid);
		if (status != SGX_SUCCESS) {
			std::cerr << "sgx_init_quote failed" << std::endl;
			exit(status);
		}
	}
}

void duk_enclave_post_quote_report(sgx_report_t *report) {
	uint32_t quote_size;
	{
		const sgx_status_t status = sgx_get_quote_size(NULL, &quote_size);
		if (status != SGX_SUCCESS) {
			std::cerr << "sgx_get_quote_size failed" << std::endl;
			exit(status);
		}
	}
	char *quote_memory = new char[quote_size];
	sgx_quote_t *quote = reinterpret_cast<sgx_quote_t *>(quote_memory);
	{
		const sgx_spid_t spid = {""};
		const sgx_status_t status = sgx_get_quote(report, SGX_UNLINKABLE_SIGNATURE, &spid, NULL, NULL, 0, NULL, quote, quote_size);
		if (status != SGX_SUCCESS) {
			std::cerr << "sgx_get_quote failed" << std::endl;
			exit(status);
		}
	}
	std::cout << "received quote" << std::endl;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " signed-enclave" << std::endl;
		return 1;
	}
	sgx_enclave_id_t enclave_id;
	{
		sgx_launch_token_t launch_token;
		int launch_token_updated;
		const sgx_status_t status = sgx_create_enclave(argv[1], SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &enclave_id, NULL);
		if (status != SGX_SUCCESS) {
			std::cerr << "sgx_create_enclave failed" << std::endl;
			exit(status);
		}
	}
	{
		const sgx_status_t status = duk_enclave_init(enclave_id, "main.js");
		if (status != SGX_SUCCESS) {
			std::cerr << "duk_enclave_init failed" << std::endl;
			exit(status);
		}
	}
	{
		const sgx_status_t status = duk_enclave_emit_message(enclave_id, "{\"signature\":\"" SIG_HEX "\",\"data\":\"" DATA_HEX "\"}");
		if (status != SGX_SUCCESS) {
			std::cerr << "duk_enclave_emit_message failed" << std::endl;
			exit(status);
		}
	}
	{
		const sgx_status_t status = duk_enclave_close(enclave_id);
		if (status != SGX_SUCCESS) {
			std::cerr << "duk_enclave_close failed" << std::endl;
			exit(status);
		}
	}
	{
		const sgx_status_t status = sgx_destroy_enclave(enclave_id);
		if (status != SGX_SUCCESS) {
			std::cerr << "sgx_destroy_enclave failed" << std::endl;
			exit(status);
		}
	}
	return 0;
}

