var SecureWorkerInternal = require('./Release/secureworker_internal');
SecureWorkerInternal.handlers.postMessage = function (message) {
    console.log('from enclave:', message);
};

var w = new SecureWorkerInternal('./Debug/duk_enclave.signed.dll');
w.init(0);
var x = new SecureWorkerInternal('./Debug/duk_enclave.signed.dll');
x.init(0);

w.emitMessage('asdf');
x.emitMessage('xxxx');
w.emitMessage('test');

w.close();
x.close();
