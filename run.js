var SecureWorkerInternal = require('./Release/secureworker_internal');

var w = new SecureWorkerInternal('./Debug/duk_enclave.signed.dll');
w.handlePostMessage = function (message) {
    console.log('from w:', message);
};
w.init(0);
w.emitMessage('hello');
w.close();
