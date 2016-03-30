var events = require('events');
var SecureWorkerInternal = require('../build/Release/secureworker_internal');
var SecureWorker = function (key) {
  var self = this;
  self._events = new events.EventEmitter();
  self._internal = new SecureWorkerInternal('../trusted/duk_enclave.signed.so');
  self._internal.handlePostMessage = function (marshalledMessage) {
    var message = JSON.parse(marshalledMessage);
    setImmediate(function () {
      self._events.emit('message', message);
    });
  };
  self._internal.init(key);
};
SecureWorker.prototype.onMessage = function (listener) {
  this._events.addListener('message', listener);
};
SecureWorker.prototype.removeOnMessage = function (listener) {
  this._events.removeListener('message', listener);
};
SecureWorker.prototype.postMessage = function (message) {
  var self = this;
  var marshalledMessage = JSON.stringify(message);
  setImmediate(function () {
    self._internal.emitMessage(marshalledMessage);
  });
};
SecureWorker.prototype.terminate = function () {
  var self = this;
  setImmediate(function () {
    self._internal.close();
    self._internal = null;
  });
};
module.exports = SecureWorker;
