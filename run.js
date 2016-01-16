var m = require('./Release/secureworker_internal');
m.handlers.postMessage = function (message) {
    console.log('got back', message);
};
m.native.emitMessage('asdf');
m.native.emitMessage('test');
