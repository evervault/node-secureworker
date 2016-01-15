var m = require('./Release/m');
m._handlers.postMessage = function (message) {
    console.log('got back', message);
};
m._emitMessage('asdf');
m._emitMessage('test');
