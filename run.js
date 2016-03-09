var SecureWorkerInternal = require('./build/Release/secureworker_internal');

var SIG_HEX = 'CB28E0999B9C7715FD0A80D8E47A77079716CBBF917DD72E97566EA1C066957C86FA3BB4E26CAD5BF90B7F81899256CE7594BB1EA0C89212748BFF3B3D5B0315';
var DATA_HEX = '616263';
var w = new SecureWorkerInternal('duk_enclave/duk_enclave.signed.so');
w.handlePostMessage = function (message) {
    console.log('from w:', message);
};
w.handlePostQuote = function (quote) {
    console.log('quote from w:', new Buffer(new Uint8Array(quote)).toString('hex'));
};
w.init('main.js');
w.emitMessage(JSON.stringify({
    signature: SIG_HEX,
    data: DATA_HEX,
}));
w.close();
