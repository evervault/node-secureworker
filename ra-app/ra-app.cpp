#include "sgx_urts.h"

#include "duk_enclave_u.h"

int main(int argx, char *argv[]) {
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
	sgx_ra_context_t ra_context;
	{
		const sgx_status_t status = duk_enclave_init_ra(&ra_context, 1);
		if (status != SGX_SUCCESS) {
			std::cerr << "duk_enclave_init_ra failed" << std::endl;
			exit(status);
		}
	}
	sgx_ra_msg1_t ra_msg1;
	{
		const sgx_status_t status = sgx_ra_get_msg1(context, enclave_id, sgx_ra_get_ga, &ra_msg1);
		if (status != SGX_SUCCESS) {
			std::cerr << "sgx_ra_get_ga failed" << std::endl;
			exit(status);
		}
	}
}
