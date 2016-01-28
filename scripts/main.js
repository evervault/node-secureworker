_dukEnclaveNative.importScript('framework.js');
_dukEnclaveHandlers.emitMessage = function (message) {
	var messageBuffer = Duktape.Buffer(message, true);
	var digest = _dukEnclaveNative.sha256Digest(messageBuffer);
	var digestBuffer = Duktape.Buffer(digest);
	var digestHex = Duktape.enc('hex', digestBuffer);
	_dukEnclaveNative.postMessage(digestHex);
};
