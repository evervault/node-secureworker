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
		const sgx_status_t status = duk_enclave_init(enclave_id, "main.js");
		if (status != SGX_SUCCESS) {
			std::cerr << "duk_enclave_init failed" << std::endl;
			exit(status);
		}
	}
	{
//                                                                                  v
// #define SIG_HEX "3629ea2ac4b56561e6d92f85ed70972bc34674bec01b9671204ea6f9c8af55f8107130b13a60b1a3d04f22d303d1e86ca7e311635f1328bea94588a8be69f518"
   #define SIG_HEX "f855afc8f9a64e2071961bc0be7446c32b9770ed852fd9e66165b5c42aea293618f569bea88845a9be28135f6311e3a76ce8d103d3224fd0a3b1603ab1307110"
		const sgx_status_t status = duk_enclave_emit_message(enclave_id, "{\"signature\":\"" SIG_HEX "\",\"data\":\"68656c6c6f20776f726c64\"}");
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

