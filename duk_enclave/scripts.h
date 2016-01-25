const size_t MAX_SCRIPT = 1;
const char *SCRIPTS[MAX_SCRIPT] = {
	"_dukEnclaveHandlers.emitMessage = function (message) {\n"
	"	_dukEnclaveNative.postMessage(\n"
	"		Duktape.enc('hex', 'a')\n"
	"	);\n"
	"	_dukEnclaveNative.postMessage(\n"
	"		Duktape.enc('hex',\n"
	"			_dukEnclaveNative.encodeString(message)\n"
	"		)\n"
	"	);\n"
	"	_dukEnclaveNative.postMessage(\n"
	"		Duktape.enc('hex',\n"
	"			_dukEnclaveNative.sha256Digest(\n"
	"				_dukEnclaveNative.encodeString(message)\n"
	"			)\n"
	"		)\n"
	"	);\n"
	"};\n",
};
