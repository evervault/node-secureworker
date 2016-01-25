// sample-client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>

#include "sgx_urts.h"
#include "duk_enclave_u.h"

void duk_enclave_post_message(const char *message) {
	std::cout << message << std::endl;
}

int _tmain(int argc, _TCHAR* argv[]) {
	sgx_enclave_id_t enclave_id;
	{
		sgx_launch_token_t launch_token;
		int launch_token_updated;
		const sgx_status_t status = sgx_create_enclave(_T("duk_enclave.signed.dll"), SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &enclave_id, NULL);
		if (status != SGX_SUCCESS) {
			std::cerr << "sgx_create_enclave failed" << std::endl;
			exit(status);
		}
	}
	{
		const sgx_status_t status = duk_enclave_init(enclave_id, 0);
		if (status != SGX_SUCCESS) {
			std::cerr << "duk_enclave_init failed" << std::endl;
			exit(status);
		}
	}
	{
		const sgx_status_t status = duk_enclave_emit_message(enclave_id, "");
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

