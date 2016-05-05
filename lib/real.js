var SecureWorkerInternal = require('../build/Release/secureworker_internal');
var MessagePort = require('./message-port');

var SecureWorker = function SecureWorker(contentKey) {
  var self = this;

  if (!(self instanceof SecureWorker)) {
    return new SecureWorker.apply(null, arguments);
  }

  self.createdCallback(contentKey);
};

SecureWorker.prototype.createdCallback = function createdCallback(contentKey) {
  var self = this;

  self._eventsFromInside = new MessagePort();
  self._internal = new SecureWorkerInternal('../trusted/duk_enclave.signed.so');
  self._internal.handlePostMessage = function (marshalledMessage) {
    var message = JSON.parse(marshalledMessage);
    setImmediate(function () {
      self._eventsFromInside.emit('message', message);
    });
  };
  self._internal.init(contentKey);
};

SecureWorker.prototype.onMessage = function onMessage(listener) {
  this._eventsFromInside.addListener('message', listener);
  this._eventsFromInside.start();
};

SecureWorker.prototype.removeOnMessage = function (listener) {
  this._eventsFromInside.removeListener('message', listener);
};

SecureWorker.prototype.postMessage = function postMessage(message) {
  var self = this;
  var marshalledMessage = JSON.stringify(message);
  setImmediate(function () {
    self._internal.emitMessage(marshalledMessage);
  });
};

SecureWorker.prototype.terminate = function terminate() {
  var self = this;
  setImmediate(function () {
    self._internal.close();
    self._internal = null;
  });
};

SecureWorker.Internal = SecureWorkerInternal;

module.exports = SecureWorker;
