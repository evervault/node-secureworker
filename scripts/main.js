_dukEnclaveNative.importScript('framework.js');
_dukEnclaveHandlers.emitMessage = function (marshalledMessage) {
	var message = JSON.parse(marshalledMessage);
	var messageBuffer = Duktape.Buffer(message, true);
	var digest = _dukEnclaveNative.sha256Digest(messageBuffer);
	var digestBuffer = Duktape.Buffer(digest);
	var digestHex = Duktape.enc('hex', digestBuffer);
	var marshalledDigestHex = JSON.stringify(digestHex);
	_dukEnclaveNative.postMessage(marshalledDigestHex);
};
