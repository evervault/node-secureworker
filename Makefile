######## SGX SDK Settings ########

SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= SIM
SGX_ARCH ?= x64

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
	SGX_COMMON_CFLAGS += -O0 -g
else
	SGX_COMMON_CFLAGS += -O2
endif

######## App Settings ########

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

App_Include_Paths := -I$(SGX_SDK)/include

App_C_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes $(App_Include_Paths)

# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
	App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
	App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

App_Cpp_Flags := $(App_C_Flags) -std=c++11
App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lpthread

ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif

######## Enclave Settings ########

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
	Crypto_Library_Name := sgx_tcrypto
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
	Crypto_Library_Name := sgx_tcrypto_opt
endif

Enclave_Include_Paths := -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stlport

Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) -std=c++03 -nostdinc++
Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tstdcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
#	-Wl,--version-script=Enclave/Enclave.lds

ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif

######## node-secureworker ########

all: duk_enclave/duk_enclave.signed.so

duktape_dist_c := $(wildcard duktape-1.4.0/src-separate/*.c)
duktape_dist_o := $(duktape_dist_c:.c=.o)

duktape-1.4.0/src-separate/%.o: CXXFLAGS+=$(Enclave_Cpp_Flags) -DDUK_OPT_NO_FILE_IO -DDUK_OPT_CPP_EXCEPTIONS -DDUK_OPT_NO_JX
duktape-1.4.0/src-separate/%.o: duktape-1.4.0/src-separate/%.c
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

scripts_js := $(wildcard scripts/*.js)

scripts/scripts-binary.o: $(scripts_js)
	cd scripts && ld -r -b binary $(patsubst scripts/%,%,$^) -o scripts-binary.o

scripts/scripts-table.o: scripts/scripts.h

duk_enclave/duk_enclave_private.pem:
	openssl genrsa -out $@ -3 3072

%_t.c %_h.c: %.edl
	cd $(<D) && $(SGX_EDGER8R) --trusted $(<F) --search-path $(SGX_SDK)/include

duk_enclave/duk_enclave_t.o: CFLAGS+=$(Enclave_C_Flags)

duk_enclave/duk_enclave.o: CXXFLAGS+=$(Enclave_Cpp_Flags) -Iduktape-1.4.0/src-separate -Iscripts
duk_enclave/duk_enclave.o: duk_enclave/duk_enclave_t.h duktape-1.4.0/src-separate/duktape.h scripts/scripts.h

duk_enclave/duk_enclave.so: LDLIBS+=$(Enclave_Link_Flags)
duk_enclave/duk_enclave.so: $(duktape_dist_o) scripts/scripts-binary.o scripts/scripts-table.o duk_enclave/duk_enclave_t.o duk_enclave/duk_enclave.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

duk_enclave/duk_enclave.signed.so: duk_enclave/duk_enclave.so duk_enclave/duk_enclave_private.pem duk_enclave/duk_enclave.config.xml
	$(SGX_ENCLAVE_SIGNER) sign \
		-key duk_enclave/duk_enclave_private.pem \
		-enclave $< \
		-out $@ \
		-config duk_enclave/duk_enclave.config.xml

.PHONY: all
