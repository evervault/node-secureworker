var configuration = process.argv[2] || 'Debug';

var SecureWorkerInternal = require('./' + configuration + '/secureworker_internal');

var w = new SecureWorkerInternal('./' + configuration + '/duk_enclave.signed.dll');
w.handlePostMessage = function (message) {
    console.log('from w:', message);
};
w.init('main.js');
w.emitMessage('abc');
w.emitMessage('');
w.close();
