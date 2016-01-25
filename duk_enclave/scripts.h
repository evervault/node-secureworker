const size_t MAX_SCRIPT = 1;
const char *SCRIPTS[MAX_SCRIPT] = {
	"_dukEnclaveHandlers.emitMessage = function (message) {\n"
	"	var messageBuffer = Duktape.Buffer(message, true);\n"
	"	var digest = _dukEnclaveNative.sha256Digest(messageBuffer);\n"
	"	var digestBuffer = Duktape.Buffer(digest);\n"
	"	var digestHex = Duktape.enc('hex', digestBuffer);\n"
	"	_dukEnclaveNative.postMessage(digestHex);\n"
	"};\n",
};
