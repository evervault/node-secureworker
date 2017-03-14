{
	"targets": [
		{
			"target_name": "secureworker_internal",
			"sources": [
				"node-secureworker-internal/secureworker-internal.cc",
				"../duk_enclave/duk_enclave_u.o",
			],
			"cflags": [
				"-std=c++11",
				"-g",
			],
			"cflags_cc!": [
				"-fno-exceptions",
			],
			"include_dirs": [
				"$(SGX_SDK)/include",
				"duk_enclave",
				"<!(node -e \"require('nan')\")",
				"<(node_root_dir)/deps/openssl/openssl/include"
			],
			"link_settings": {
				"libraries": [
					"-L$(SGX_LIBRARY_PATH)",
					"-l$(SGX_URTS_LIBRARY_NAME)",
					"-l$(SGX_SERVICE_LIBRARY_NAME)",
				],
			},
		},
	],
}
