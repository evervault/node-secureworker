var SecureWorker = require('../lib/index');

function worker() {
  F.onMessage(function (message) {
    F.postMessage(message + 1);
  });
}

SecureWorker._resolveContentKey = function _resolveContentKey(scriptKey) {
  // Ignoring scriptKey.
  return '(' + worker.toString() + ')();';
};

var secureWorker = new SecureWorker('foobar');

secureWorker.onMessage(function (message) {
  console.log("message", message);
});

secureWorker.postMessage(4);
