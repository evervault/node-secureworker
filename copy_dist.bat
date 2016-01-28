@echo off
if "%1" == "" goto usage
if "%2" == "" goto usage

copy "%~dp0duk_enclave\duk_enclave.config.xml" "%2"
copy "%~dp0duk_enclave\%1\duk_enclave.obj" "%2"
copy "%~dp0duk_enclave\%1\duk_enclave_t.obj" "%2"
copy "%~dp0%1\duktape-dist.lib" "%2"
copy "%~dp0scripts\scripts.h" "%2"
copy "%~dp0dist-files\build_enclave.bat" "%2"
goto done

:usage
echo 1>&2 usage: %0 configuration destination

:done
