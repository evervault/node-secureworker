var SecureWorkerInternal = require('./Release/secureworker_internal');

var w = new SecureWorkerInternal('./Debug/duk_enclave.signed.dll');
w.handlePostMessage = function (message) {
    console.log('from w:', message);
};
w.init(0);
var x = new SecureWorkerInternal('./Debug/duk_enclave.signed.dll');
x.handlePostMessage = function (message) {
    console.log('from x:', message);
};
x.init(0);

w.emitMessage('asdf');
x.emitMessage('xxxx');
w.emitMessage('test');

w.close();
x.close();

w = null;
x = null;

if (typeof gc === 'function') gc();
