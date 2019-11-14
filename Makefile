######## SGX SDK Settings ########

SGX_SDK ?= /opt/intel/sgxsdk
SGX_DEBUG ?= 1
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
	Uae_Service_Library_Name := sgx_uae_service_sim
else
	Urts_Library_Name := sgx_urts
	Uae_Service_Library_Name := sgx_uae_service
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
App_Link_Flags := $(SGX_COMMON_CFLAGS) -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lpthread -l$(Uae_Service_Library_Name)

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

Enclave_Include_Paths := -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/stdc++

Enclave_C_Flags := $(SGX_COMMON_CFLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) -std=c++03 -nostdinc++
Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tcxx -lsgx_tkey_exchange -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \

ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
	Build_Mode = HW_RELEASE
endif
endif
endif

define DEFAULT_ENCLAVE_CONFIG
<EnclaveConfiguration>
  <ProdID>0</ProdID>
  <ISVSVN>0</ISVSVN>
  <StackMaxSize>0x40000</StackMaxSize>
  <HeapMaxSize>0x100000</HeapMaxSize>
  <TCSNum>1</TCSNum>
  <TCSPolicy>1</TCSPolicy>
  <DisableDebug>0</DisableDebug>
  <MiscSelect>0</MiscSelect>
  <MiscMask>0xFFFFFFFF</MiscMask>
</EnclaveConfiguration>
endef
export DEFAULT_ENCLAVE_CONFIG

######## node-secureworker ########

all:
	@echo 'Run "npm run build" or "make enclave".'

node: Makefile build/Release/secureworker_internal.node enclave-autoexec/autoexec.js

%_u.c %_u.h: %.edl
	cd $(<D) && $(SGX_EDGER8R) --untrusted $(<F) --search-path $(SGX_SDK)/include

duktape_dist_c := $(wildcard duktape-1.4.0/src-separate/*.c)
duktape_dist_o := $(duktape_dist_c:.c=.o)

duktape-1.4.0/src-separate/%.o: CXXFLAGS += $(Enclave_Cpp_Flags) -DDUK_OPT_NO_FILE_IO -DDUK_OPT_CPP_EXCEPTIONS -DDUK_OPT_NO_JX
duktape-1.4.0/src-separate/%.o: duktape-1.4.0/src-separate/%.c
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

duk_enclave/duk_enclave_u.o: CFLAGS += $(App_C_Flags)

build:
	@mkdir -p build

build/Release/secureworker_internal.node: export SGX_SDK := $(SGX_SDK)
build/Release/secureworker_internal.node: export SGX_LIBRARY_PATH := $(SGX_LIBRARY_PATH)
build/Release/secureworker_internal.node: export SGX_URTS_LIBRARY_NAME := $(Urts_Library_Name)
build/Release/secureworker_internal.node: export SGX_SERVICE_LIBRARY_NAME := $(Service_Library_Name)
build/Release/secureworker_internal.node: node-secureworker-internal/secureworker-internal.cc duk_enclave/duk_enclave_u.h duk_enclave/duk_enclave_u.o | build
	node-gyp rebuild

enclave-autoexec/autoexec.js: enclave-autoexec/index.js node_modules/promise-polyfill/promise.js
	./node_modules/.bin/browserify --insert-global-vars __filename,__dirname --no-commondir $< > $@

######## enclave building ########

# Example:
#
#   make enclave SCRIPTS='tests/main.js another/script.js' ENCLAVE_KEY='key.pem' ENCLAVE_OUTPUT='enclave-main.so'
#
# Scripts will be exposed as "main.js" and "script.js" inside the enclave (their basename).
# Enclave key will be generated for you, if it does not yet exist.
# If you do not provide ENCLAVE_OUTPUT, enclave will be output to "build/enclave.so".

ENCLAVE_OUTPUT ?= build/enclave.so
ENCLAVE_OUTPUT_UNSIGNED ?= $(addsuffix $(addprefix .unsigned,$(suffix $(ENCLAVE_OUTPUT))),$(basename $(ENCLAVE_OUTPUT)))
ENCLAVE_CONFIG ?= build/enclave.config.xml
ENCLAVE_KEY ?= key.pem

# A rule to check if output from generate-scripts-table changed based on SCRIPTS. Quietly.
scripts/scripts-table.c.changed: scripts/generate-scripts-table.sh always-rebuild
	@scripts/generate-scripts-table.sh enclave-autoexec/autoexec.js ${SCRIPTS} > $@.tmp
	@[ -e $@ ] && diff -q $@ $@.tmp > /dev/null || cp $@.tmp $@
	@rm -f $@.tmp

scripts/scripts-table.c: scripts/generate-scripts-table.sh scripts/scripts-table.c.changed
	@if [ -z "${SCRIPTS}" ]; then echo "You have to pass list of SCRIPTS to build into the enclave: make enclave SCRIPTS='worker1.js worker2.js'"; exit 1; fi
	scripts/generate-scripts-table.sh enclave-autoexec/autoexec.js ${SCRIPTS} > $@

# A rule to check if output from generate-scripts-data changed based on SCRIPTS. Quietly.
scripts/scripts-binary.as.changed: scripts/generate-scripts-data.sh always-rebuild
	@scripts/generate-scripts-data.sh enclave-autoexec/autoexec.js ${SCRIPTS} > $@.tmp
	@[ -e $@ ] && diff -q $@ $@.tmp > /dev/null || cp $@.tmp $@
	@rm -f $@.tmp

scripts/scripts-binary.as: scripts/generate-scripts-data.sh scripts/scripts-binary.as.changed
	@if [ -z "${SCRIPTS}" ]; then echo "You have to pass list of SCRIPTS to build into the enclave: make enclave SCRIPTS='worker1.js worker2.js'"; exit 1; fi
	scripts/generate-scripts-data.sh enclave-autoexec/autoexec.js ${SCRIPTS} > $@

scripts/scripts-binary.o: scripts/scripts-binary.as enclave-autoexec/autoexec.js $(SCRIPTS)
	as $< -o $@

scripts/scripts-table.o: scripts/scripts-table.c scripts/scripts.h

%_t.c %_t.h: %.edl
	cd $(<D) && $(SGX_EDGER8R) --trusted $(<F) --search-path $(SGX_SDK)/include

duk_enclave/duk_enclave_t.o: CFLAGS += $(Enclave_C_Flags)

duktape-1.4.0/libduktape.a: $(duktape_dist_o)
	ar -rcs $@ $^

duk_enclave/duk_enclave.o: CXXFLAGS += $(Enclave_Cpp_Flags) -Iduktape-1.4.0/src-separate -Iscripts
duk_enclave/duk_enclave.o: duk_enclave/duk_enclave_t.h duktape-1.4.0/src-separate/duktape.h scripts/scripts.h

${ENCLAVE_OUTPUT_UNSIGNED}: LDLIBS += $(Enclave_Link_Flags)
${ENCLAVE_OUTPUT_UNSIGNED}: scripts/scripts-binary.o scripts/scripts-table.o duk_enclave/duk_enclave_t.o duk_enclave/duk_enclave.o duktape-1.4.0/libduktape.a
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

${ENCLAVE_OUTPUT}: ${ENCLAVE_OUTPUT_UNSIGNED} ${ENCLAVE_KEY} ${ENCLAVE_CONFIG}
	$(SGX_ENCLAVE_SIGNER) sign \
		-key ${ENCLAVE_KEY} \
		-enclave $< \
		-out $@ \
		-config ${ENCLAVE_CONFIG}

${ENCLAVE_KEY}:
	@echo "Generating a signing key and storing it into '${ENCLAVE_KEY}'."
	openssl genrsa -out $@ -3 3072

${ENCLAVE_CONFIG}:
	@echo "Storing a default enclave signing configuration into '${ENCLAVE_CONFIG}'."
	echo "$$DEFAULT_ENCLAVE_CONFIG" > $@

enclave: node Makefile build ${ENCLAVE_OUTPUT}

.PHONY: all build always-rebuild
.SECONDARY: duk_enclave/duk_enclave_t.c duk_enclave/duk_enclave_t.h duk_enclave/duk_enclave_u.c duk_enclave/duk_enclave_u.h
.PRECIOUS: %.key %.config.xml