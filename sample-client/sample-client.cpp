// sample-client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <chrono> // %%%
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
	for (size_t i = 0; i < 26; i++) {
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		{
			const sgx_status_t status = duk_enclave_init(enclave_id, i);
			if (status != SGX_SUCCESS) {
				std::cerr << "duk_enclave_init failed" << std::endl;
				exit(status);
			}
		}
		std::chrono::steady_clock::time_point finish = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration duration = finish - start;
		std::cerr << std::fixed << std::chrono::duration<double, std::micro>(duration).count() << " us" << std::endl;
		{
			const sgx_status_t status = duk_enclave_close(enclave_id);
			if (status != SGX_SUCCESS) {
				std::cerr << "duk_enclave_close failed" << std::endl;
				exit(status);
			}
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

