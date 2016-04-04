var thirty_two = new Uint8Array(32);
console.log('slice is', typeof thirty_two.slice); // %%%

// Test vector from RFC 5903
var keyA = {
	privateHex: 'C88F01F510D9AC3F70A292DAA2316DE544E9AAB8AFE84049C62A9C57862D1433',
	publicHex: 'DAD0B65394221CF9B051E1FECA5787D098DFE637FC90B9EF945D0C37725811805271A0461CDB8252D61F1C456FA3E59AB1F45B33ACCF5F58389E0577B8990BB3',
};
var keyB = {
	privateHex: 'C6EF9C5D78AE012A011164ACB397CE2088685D8F06BF9BE0B283AB46476BEE53',
	publicHex: 'D12DFB5289C8D4F81208B70270398C342296970A0BCCB74C736FC7554494BF6356FBF3CA366CC23E8157854C13C58D6AAC23F046ADA30F8353E74F33039872AB',
};
var algorithm = {
	name: 'ECDH',
	namedCurve: 'P-256',
};
var report = function (reason) {
	console.log('rejected:', reason.stack || reason);
}
Promise.all([
	crypto.subtle.importKey('raw-private-be', Duktape.dec('hex', keyA.privateHex), algorithm, true, ['deriveBits']),
	crypto.subtle.importKey('raw-public-uncompressed-be', Duktape.dec('hex', keyA.publicHex), algorithm, true, []),
	crypto.subtle.importKey('raw-private-be', Duktape.dec('hex', keyB.privateHex), algorithm, true, ['deriveBits']),
	crypto.subtle.importKey('raw-public-uncompressed-be', Duktape.dec('hex', keyB.publicHex), algorithm, true, []),
]).then(function (keys) {
	keyA.privateKey = keys[0];
	keyA.publicKey = keys[1];
	keyB.privateKey = keys[2];
	keyB.publicKey = keys[3];
	crypto.subtle.exportKey('pkcs8', keyA.privateKey).then(function (result) {
		console.log('A private', Duktape.enc('hex', Duktape.Buffer(result)));
	}).catch(report);
	crypto.subtle.exportKey('spki', keyA.publicKey).then(function (result) {
		console.log('A public', Duktape.enc('hex', Duktape.Buffer(result)));
	}).catch(report);
	crypto.subtle.exportKey('pkcs8', keyB.privateKey).then(function (result) {
		console.log('B private', Duktape.enc('hex', Duktape.Buffer(result)));
	}).catch(report);
	crypto.subtle.exportKey('spki', keyB.publicKey).then(function (result) {
		console.log('B public', Duktape.enc('hex', Duktape.Buffer(result)));
	}).catch(report);
	crypto.subtle.deriveBits({name: 'ECDH', public: keyB.publicKey}, keyA.privateKey, 256).then(function (result) {
		console.log('A <- B =', Duktape.enc('hex', Duktape.Buffer(result)));
	}).catch(report);
	crypto.subtle.deriveBits({name: 'ECDH', public: keyA.publicKey}, keyB.privateKey, 256).then(function (result) {
		console.log('B <- A =', Duktape.enc('hex', Duktape.Buffer(result)));
	}).catch(report);
}).catch(report);

/*
var clear = new Uint8Array([1, 2, 3]);
var sealed = _dukEnclaveNative.sealData(null, clear);
var basic = Array.prototype.slice.call(new Uint8Array(sealed));
F.postMessage(basic);

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
	}).catch(function (reason) {
		console.log('rejected', reason.stack || reason);
	});
});
*/
