var SecureWorkerInternal = require('../build/Release/secureworker_internal');
var MessagePort = require('./message-port');

var SecureWorker = function SecureWorker(enclaveName, contentKey) {
  var self = this;

  if (!(self instanceof SecureWorker)) {
    return new SecureWorker.apply(null, arguments);
  }

  self.createdCallback(enclaveName, contentKey);
};

SecureWorker._Internal = SecureWorkerInternal;

SecureWorker.prototype.createdCallback = function createdCallback(enclaveName, contentKey) {
  var self = this;

  self._eventsFromInside = new MessagePort();

  self._internal = new SecureWorker._Internal(enclaveName);

  self._internal.handlePostMessage = function (marshalledMessage) {
    var message = JSON.parse(marshalledMessage);
    setImmediate(function () {
      self._eventsFromInside.emit('message', message);
    });
  };

  self._internal.init(contentKey);
};

SecureWorker.prototype.onMessage = function onMessage(listener) {
  var self = this;

  self._eventsFromInside.addListener('message', listener);
  self._eventsFromInside.start();

  return listener;
};

SecureWorker.prototype.removeOnMessage = function (listener) {
  var self = this;

  self._eventsFromInside.removeListener('message', listener);
};

SecureWorker.prototype.postMessage = function postMessage(message) {
  var self = this;
  var marshalledMessage = JSON.stringify(message);
  // We want to simulate asynchronous messaging.
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

// Extracts report data (64 bytes of extra information) from a report.
// Inputs and outputs are an arraybuffer.
SecureWorker.getReportData = function getReportData(report) {
  throw new Error("Not implemented.");
};

// Converts a report (something which can be checked locally) to a quote (something which
// can be checked remotely). Report should come from this machine.
// Inputs and outputs are an arraybuffer.
SecureWorker.getQuote = function getQuote(report) {
  throw new Error("Not implemented.");
};

// Extracts report data (64 bytes of extra information) from a quote.
// Inputs and outputs are an arraybuffer.
SecureWorker.getQuoteData = function getQuoteData(quote) {
  throw new Error("Not implemented.");
};

// Do a remote attestation of a given quote. Quote can come from this or some other machine.
// Returns a signed remote attestation statement on successful remote attestation.
// Inputs and outputs are an arraybuffer.
SecureWorker.getRemoteAttestation = function getRemoteAttestation(quote) {
  throw new Error("Not implemented.");
};

// Validates a signed remote attestation statement for a given quote.
// Input is an arraybuffer, output is true or false.
SecureWorker.validateRemoteAttestation = function validateRemoteAttestation(quote, attestation) {
  throw new Error("Not implemented.");
};

module.exports = SecureWorker;
