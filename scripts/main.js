// Test vector from RFC 4754
var clear = new Uint8Array([1, 2, 3]);
var sealed = _dukEnclaveNative.sealData(null, clear);
var basic = Array.prototype.slice.call(new Uint8Array(sealed));
F.postMessage(basic);

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
    }).catch(function (reason) {
        console.log('rejected', reason.stack || reason);
    });
});
