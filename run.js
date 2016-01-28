// TODO: Figure out the configuration??
var SecureWorkerInternal = require('./Debug/secureworker_internal');

var w = new SecureWorkerInternal('./duk_enclave.signed.dll');
w.handlePostMessage = function (message) {
    console.log('from w:', message);
};
w.init(0);
w.emitMessage('abc');
w.emitMessage('');
w.close();
