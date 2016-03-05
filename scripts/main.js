var setTimeout; // so Promise.js doesn't crash
_dukEnclaveNative.importScript('Promise.js');
_dukEnclaveNative.importScript('framework.js');
// Test vector from RFC 4754
var rawKeyHex = '2442A5CC0ECD015FA3CA31DC8E2BBC70BF42D60CBCA20085E0822CB04235E9706FC98BD7E50211A4A27102FA3549DF79EBCB4BF246B80945CDDFE7D509BBFD7D';
var publicKey = crypto.subtle.importKey('raw-public-uncompressed-be', Duktape.dec('hex', rawKeyHex), {
    name: 'ECDSA',
    namedCurve: 'P-256',
}, true, ['verify']);
F.onMessage(function (message) {
    publicKey.then(function (publicKey) {
        return crypto.subtle.verify({
            name: 'ECDSA',
            hash: 'SHA-256',
        }, publicKey, Duktape.dec('hex', message.signature), Duktape.dec('hex', message.data));
    }).then(function (valid) {
        F.postMessage(valid);
        _dukEnclaveNative.postQuote(new Uint8Array(64));
    }).catch(function (reason) {
        console.log('rejected', reason.stack || reason);
    });
});
