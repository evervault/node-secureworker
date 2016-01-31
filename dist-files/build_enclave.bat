@where cl || call "%VS110COMNTOOLS%VsDevCmd.bat"
@where ld || set "PATH=%PATH%;C:\MinGW\bin"
cl /c /I"C:\Program Files (x86)\Intel\IntelSGXSDK\include\tlibc" /I"%~dp0." "scripts-table.c"
ld -r -b binary -o scripts-binary.lib %*
link "/OUT:duk_enclave.dll" "/LIBPATH:C:\Program Files (x86)\Intel\IntelSGXSDK\bin\Win32\Debug" "sgx_trts.lib" "sgx_tstdc.lib" "sgx_tservice.lib" "sgx_tcrypto_opt.lib" "sgx_tstdcxx.lib" /NODEFAULTLIB /MANIFEST:NO /NOENTRY /DYNAMICBASE /NXCOMPAT /MACHINE:X86 /DLL "%~dp0duk_enclave.obj" "%~dp0duk_enclave_t.obj" "%~dp0duktape-dist.lib" "scripts-binary.lib" "scripts-table.obj"
sgx_sign sign -key "..\..\..\..\keys\app.private.pem" -enclave "duk_enclave.dll" -out "duk_enclave.signed.dll" -config "%~dp0duk_enclave.config.xml"
