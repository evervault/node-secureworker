var setTimeout; // so Promise.js doesn't crash
_dukEnclaveNative.importScript('Promise.js');
_dukEnclaveNative.importScript('framework.js');
F.onMessage(function (message) {
    var messageBuffer = Duktape.Buffer(message);
    crypto.subtle.digest('SHA-256', messageBuffer).then(function (digest) {
        var digestBuffer = Duktape.Buffer(digest);
        var digestHex = Duktape.enc('hex', digestBuffer);
        F.postMessage(digestHex);
    }).catch(function (reason) {
        console.log('rejected', reason);
    });
});
