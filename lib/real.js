var SecureWorkerInternal = require('../build/Release/secureworker_internal');
var MessagePort = require('./message-port');
var http = require('http');

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
    self._internal.destroy();
    self._internal = null;
  });
};

// Returns data needed for generation of a report and later on quote.
SecureWorker.initQuote = function initQuote() {
  return SecureWorker._Internal.initQuote();
};

// Extracts report data (64 bytes of extra information) from a report.
// Inputs and outputs are an arraybuffer.
SecureWorker.getReportData = function getReportData(report) {
  return report.slice(320, 320 + 64);
};

// Converts a report (something which can be checked locally) to a quote (something which
// can be checked remotely). Report should come from this machine.
// Inputs and outputs are an arraybuffer. "linkable" is a boolean.
SecureWorker.getQuote = function getQuote(report, linkable, spid, revocationList) {
  // TODO: If revocationList is not provided (undefined), fetch (and cache) the recent revocation list from Intel.
  return SecureWorker._Internal.getQuote(report, linkable, spid, revocationList);
};

// Extracts report data (64 bytes of extra information) from a quote.
// Inputs and outputs are an arraybuffer.
SecureWorker.getQuoteData = function getQuoteData(quote) {
  return quote.slice(48 + 320, 48 + 320 + 64);
};

// Do a remote attestation of a given quote. Quote can come from this or some other machine.
// Returns a signed remote attestation statement on successful remote attestation.
// Input is:
//  - an arraybuffer containing the quote
//  - a payload object containing the subscription key (mandatory), pseManifest (optional), nonce (optional)
//  - the url of the remoteAttestation server (empty strings will default to intel at api.trustedservices.intel.com)
//  - a callback function structured at function(result,err)
// The RA signature is returned using the callback function as an arraybuffer
// If an error occurs, result will be undefined and err will contain a string
SecureWorker.getRemoteAttestation = function getRemoteAttestation(quote, payloadObj, raUrl, callbackFn) {
  var self=this;

  var quoteData = self.getQuoteData(quote);
  var quoteStr = String.fromCharCode.apply(null,new UInt16Array(quoteData));

  //check for missing parameter or empty string & set to default
  if(!raUrl) {
    raUrl = "https://api.trustedservices.intel.com/sgx/attestation/v3/report";
  }

  if(!payloadObj.subscriptionKey) {
    throw new Error("Subscription key must be provided for remote attestation");
  }
  const subscriptionKey = payloadObj.subscriptionKey;

  const payload = {"isvEnclaveQuote":quoteStr};

  var pseManifest;
  var nonce;
  
  if(payloadObj.pseManifest) {
    payload.pseManifest = payloadObj.pseManifest;
  }

  if(payloadObj.nonce) {
    payload.nonce = payloadObj.nonce;
  }

  const options = {
    method: "POST",
    headers: {
      "Ocp-Apim-Subscription-Key": subscriptionKey
    }
  };

  const handleResponse = function(res) {
    if(res) {

      switch(res.statusCode) {

      case 200:
        var raSig = res.headers["X-IASReport-Signature"];
        if(raSig && raSig.length > 0){
          
          //convert ra_sig to an arraybuffer
          // 2 bytes for each char
          var sigBuffer = new ArrayBuffer(raSig.length*2);
          var bufView = new Uint16Array(sigBuffer);

          for (var i=0; i < raSig.length; i++) {
              bufView[i] = raSig.charCodeAt(i);
          }

          callbackFn(sigBuffer);
        }

        break;

      case 400:
        callbackFn(undefined,"Bad Request: Invalid Attestation Evidence Payload");
        break;

      case 401:
        callbackFn(undefined, "Unauthorized: failed to authenticate");
        break;

      case 500:
        callbackFn(undefined, "Internal Server Error");
        break;

      case 503:
        callbackFn(undefined, "Service Unavailable");
        break;

      default:
        callbackFn(undefined, "Unknown Error - Report this");
        break;

      }

    } else{
      callbackFn(undefined, "Invalid Response");
    }
  }

  var req = http.request(raUrl, options, handleResponse);
  req.write(payload);
  req.end();
  return;
};

// Validates a signed remote attestation statement for a given quote.
// Input is an arraybuffer, output is true or false.
SecureWorker.validateRemoteAttestation = function validateRemoteAttestation(quote, attestation) {
  throw new Error("Not implemented.");
};

SecureWorker.getSGXVersion = function getSGXVersion() {
  // TODO: Return real information about SGX version and version of services (and their enclaves) it provides.
  return {};
};

module.exports = SecureWorker;
