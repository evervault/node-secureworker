// sample-client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include "sgx_urts.h"
#include "duktape-enclave_u.h"

int _tmain(int argc, _TCHAR* argv[]) {
	sgx_status_t status;
	sgx_launch_token_t launch_token;
	int launch_token_updated;
	sgx_enclave_id_t enclave_id;
	status = sgx_create_enclave(_T("duktape-enclave.signed.dll"), SGX_DEBUG_FLAG, &launch_token, &launch_token_updated, &enclave_id, NULL);
	if (status != SGX_SUCCESS) {
		std::cerr << "sgx_create_enclave failed with " << status << std::endl;
		return -1;
	}
	std::cerr << "sgx: created enclave " << enclave_id << std::endl;

	double retval;
	status = test(enclave_id, &retval);
	if (status != SGX_SUCCESS) {
		std::cerr << "test failed with " << status << std::endl;
		return -1;
	}
	std::cerr << "test: returned " << retval << std::endl;

	status = sgx_destroy_enclave(enclave_id);
	if (status != SGX_SUCCESS) {
		std::cerr << "sgx_destroy_enclave failed with " << status << std::endl;
		return -1;
	}
	std::cerr << "sgx: destroyed enclave " << enclave_id << std::endl;

	return 0;
}

