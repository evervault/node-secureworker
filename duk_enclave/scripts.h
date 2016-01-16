const size_t MAX_SCRIPT = 2;
const char *SCRIPTS[MAX_SCRIPT] = {
	"_dukEnclaveNative.importScript(1);\n"
	"_dukEnclaveHandlers.emitMessage = function (message) {\n"
	"	var s = i++;\n"
	"	_dukEnclaveNative.nextTick(function () { _dukEnclaveNative.postMessage(message + ' (' + s + 'b)'); });\n"
	"	_dukEnclaveNative.postMessage(message + ' (' + s + ')');\n"
	"};\n",

	"var i = 100;\n",
};
