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

