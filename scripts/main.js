var setTimeout; // so Promise.js doesn't crash
_dukEnclaveNative.importScript('Promise.js');
_dukEnclaveNative.importScript('framework.js');
//                                                                                  v
// var rawKeyHex = '20EB8A970AB631E675DAAB60C8796521A6CC2671A2FE1B27D77F43FA0655726B09B4A2398B09942C27FF706C9C8BB9A61E5787A315D6EA843F7886690610DC67';
   var rawKeyHex = '6B725506FA437FD7271BFEA27126CCA6216579C860ABDA75E631B60A978AEB2067DC10066986783F84EAD615A387571EA6B98B9C6C70FF272C94098B39A2B409';
var publicKey = crypto.subtle.importKey('raw', Duktape.dec('hex', rawKeyHex), {
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
