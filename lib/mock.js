var vm = require('vm');
var Promise = require('promise-polyfill');
var WebCrypto = require('node-webcrypto-ossl');
var MessagePort = require('./message-port');
var crypto = require('crypto');
var uuid = require('node-uuid');
var Uint64LE = require('int64-buffer').Uint64LE;
var _ = require('underscore');

var SecureWorker = function SecureWorker(enclaveName, contentKey) {
  var self = this;

  if (!(self instanceof SecureWorker)) {
    return new SecureWorker.apply(null, arguments);
  }

  self.createdCallback(enclaveName, contentKey);
};

SecureWorker.prototype.createdCallback = function createdCallback(enclaveName, contentKey) {
  var self = this;

  self._eventsFromOutside = new MessagePort();
  self._eventsFromInside = new MessagePort();
  self._listenerMap = new WeakMap();

  var code = this.constructor._resolveContentKey(enclaveName, contentKey);
  var sandbox = this.constructor._sandboxContext(self, enclaveName, contentKey);

  self._context = vm.createContext(sandbox);
  self._vmGlobalContext = vm.runInContext('this', self._context);

  vm.runInContext(code, self._context, {
    filename: contentKey,
    displayErrors: true
  });
};

SecureWorker.prototype.onMessage = function onMessage(listener) {
  var self = this;

  if (!self._listenerMap.has(listener)) {
    self._listenerMap.set(listener, function (message) {
      listener(JSON.parse(message));
    });
  }

  self._eventsFromInside.addListener('message', self._listenerMap.get(listener));
  self._eventsFromInside.start();

  return listener;
};

SecureWorker.prototype.removeOnMessage = function removeOnMessage(listener) {
  var self = this;

  self._eventsFromInside.removeListener('message', self._listenerMap.get(listener));
};

SecureWorker.prototype.postMessage = function postMessage(message) {
  var self = this;
  var marshalledMessage = JSON.stringify(message);
  // We want to simulate asynchronous messaging.
  setImmediate(function () {
    self._eventsFromOutside.emit('message', marshalledMessage);
  });
};

SecureWorker.prototype.terminate = function terminate() {
  var self = this;

  // TODO: Is there a way to implement this using "vm"? If there are no timers it should be possible?
  // A noop in this mock implementation.
};

var GLOBAL_PROPERTIES = [
  'NaN',
  'Infinity',
  'undefined',
  'Object',
  'Function',
  'Array',
  'String',
  'Boolean',
  'Number',
  'Date',
  'RegExp',
  'Error',
  'EvalError',
  'RangeError',
  'ReferenceError',
  'SyntaxError',
  'TypeError',
  'URIError',
  'Math',
  'JSON',
  //'Duktape',
  //'Proxy',
  'Buffer',
  'ArrayBuffer',
  'DataView',
  'Int8Array',
  'Uint8Array',
  'Uint8ClampedArray',
  'Int16Array',
  'Uint16Array',
  'Int32Array',
  'Uint32Array',
  'Float32Array',
  'Float64Array',
  'eval',
  'parseInt',
  'parseFloat',
  'isNaN',
  'isFinite',
  'decodeURI',
  'decodeURIComponent',
  'encodeURI',
  'encodeURIComponent',
  'escape',
  'unescape',
  //'print',
  //'alert',
  //'require'
  // TODO: Remove. Only for debugging. Not available in Duktape.
  'console'
];


GLOBAL_PROPERTIES = GLOBAL_PROPERTIES.filter(function (property, i, array) {
  if (!global.hasOwnProperty(property)) {
    throw new Error("Missing property in global context: " + property);
  }

  try {
    // We check if it already exists in the empty context provided by the vm module.
    vm.runInNewContext(property);
  }
  catch (error) {
    // It does not, we have to copy it over from outside the global context.
   return true;
  }

  // It does exist, we do not have to and also should not (to not override things like Array
  // which then become different from [].constructor) copy from the outside global context.
  return false;
});

// Class method for this mock implementation to allow specifying sandbox context.
SecureWorker._sandboxContext = function _sandboxContext(secureWorker, enclaveName, contentKey) {
  var beingImportedScripts = [];
  var alreadyImportedScripts = [contentKey];

  var crypto = new WebCrypto();

  var listenerMap = new WeakMap();

  var sandbox = {
    // Our internal trusted API.
    SecureWorker: {
      ready: Promise.resolve(),

      getName: function getName() {
        return contentKey;
      },

      // Callbacks are called only after SecureWorker.ready resolves.
      onMessage: function onMessage(listener) {
        if (!listenerMap.has(listener)) {
          listenerMap.set(listener, function (message) {
            listener(secureWorker._vmGlobalContext.JSON.parse(message));
          });
        }

        secureWorker._eventsFromOutside.addListener('message', listenerMap.get(listener));

        sandbox.SecureWorker.ready.then(function () {
          secureWorker._eventsFromOutside.start();
        });

        return listener;
      },

      removeOnMessage: function removeOnMessage(listener) {
        secureWorker._eventsFromOutside.removeListener('message', listenerMap.get(listener));
      },

      postMessage: function postMessage(message) {
        var marshalledMessage = secureWorker._vmGlobalContext.JSON.stringify(message);
        // We want to simulate asynchronous messaging.
        setImmediate(function () {
          secureWorker._eventsFromInside.emit('message', marshalledMessage);
        });
      },

      close: function close() {
        secureWorker.terminate();
      },

      // In trusted environment on the server, SecureWorker.importScripts assures that
      // the script is loaded only once for a given content key.
      importScripts: function importScripts(/* args */) {
        for (var i = 0; i < arguments.length; i++) {
          var contentKey = arguments[i];

          if (alreadyImportedScripts.indexOf(contentKey) !== -1) continue;
          if (beingImportedScripts.indexOf(contentKey) !== -1) continue;
          beingImportedScripts.push(contentKey);

          try {
            var code = SecureWorker._resolveContentKey(enclaveName, contentKey);

            vm.runInContext(code, secureWorker._context, {
              filename: contentKey,
              displayErrors: true
            });

            // Successfully imported.
            alreadyImportedScripts.push(contentKey);
          }
          finally {
            var index;
            while ((index = beingImportedScripts.indexOf(contentKey)) !== -1) {
              beingImportedScripts.splice(index, 1);
            }
          }
        }
      },

      monotonicCounters: {
        // Returns an object {uuid:arraybuffer, value:number}.
        create: function create() {
          return SecureWorker._createMonotonicCounter(secureWorker._vmGlobalContext);
        },

        destroy: function destroy(counterId) {
          SecureWorker._destroyMonotonicCounter(secureWorker._vmGlobalContext, counterId);
        },

        // Returns the number.
        read: function read(counterId) {
          return SecureWorker._readMonotonicCounter(secureWorker._vmGlobalContext, counterId);
        },

        // Returns the number.
        increment: function increment(counterId) {
          return SecureWorker._incrementMonotonicCounter(secureWorker._vmGlobalContext, counterId);
        }
      },

      // Returns an object {currentTime:arraybuffer, timeSourceNonce:arraybuffer}.
      getTrustedTime: function getTrustedTime() {
        return SecureWorker._getTrustedTime(secureWorker._vmGlobalContext);
      },

      // Returns the report as arraybuffer. reportData is 64 bytes of extra information, targetInfo is 512 bytes, arraybuffers. Both optional.
      // TODO: Make it so that if targetInfo is not passed, result from initQuote is used called (instead of default useless report).
      getReport: function getReport(reportData, targetInfo) {
        return SecureWorker._getReport(secureWorker._vmGlobalContext, reportData, targetInfo);
      }
    },

    Promise: Promise,

    crypto: crypto,

    nextTick: process.nextTick,

    setImmediate: process.nextTick
  };

  sandbox.Promise._immediateFn = process.nextTick;

  sandbox.self = sandbox;

  sandbox.global = sandbox;

  for (var i = 0; i < GLOBAL_PROPERTIES.length; i++) {
    var property = GLOBAL_PROPERTIES[i];

    sandbox[property] = global[property];
  }

  return sandbox;
};

// Returns data needed for generation of a report and later on quote.
SecureWorker.initQuote = function initQuote() {
  return {
    targetInfo: new ArrayBuffer(512),
    gid: new ArrayBuffer(4)
  };
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
  if (!(report instanceof ArrayBuffer && report.byteLength === 432)) throw new Error("Invalid report.");

  if (!_.isBoolean(linkable)) throw new Error("Invalid linkable.");

  if (!(spid instanceof ArrayBuffer && spid.byteLength === 16)) throw new Error("Invalid spid.");

  // Using == on purpose.
  if (!(revocationList == null || revocationList instanceof ArrayBuffer)) throw new Error("Invalid revocation list.");

  // quote = version (2 B) + sign_type (2 B) + sgx_epid_group_id_t (4 B) + sgx_isv_svn_t (2 B) + uint8_t[6] + sgx_basename_t (32 B) + sgx_report_body_t (384 B) + signature_len (4 B) + uint8_t[]

  var reportBody = report.slice(0, 384);

  var quote = new ArrayBuffer(436);

  var view = new Uint8Array(quote);
  view.set(new Uint8Array(reportBody), 48);

  return quote;
};

// Extracts report data (64 bytes of extra information) from a quote.
// Inputs and outputs are an arraybuffer.
SecureWorker.getQuoteData = function getQuoteData(quote) {
  return quote.slice(48 + 320, 48 + 320 + 64);
};

// Do a remote attestation of a given quote. Quote can come from this or some other machine.
// Returns a signed remote attestation statement on successful remote attestation.
// Inputs and outputs are an arraybuffer.
// Override this method if you want a different mock implementation of remote attestation.
SecureWorker.getRemoteAttestation = function getRemoteAttestation(quote) {
  // To convert a string to ArrayBuffer.
  return new Uint8Array(new Buffer("mock", "utf8")).buffer;
};

// Validates a signed remote attestation statement for a given quote.
// Input is an arraybuffer, output is true or false.
// Override this method if you want a different mock implementation of remote attestation.
SecureWorker.validateRemoteAttestation = function validateRemoteAttestation(quote, attestation) {
  return true;
};

SecureWorker.getSGXVersion = function getSGXVersion() {
  // Mock returns null.
  return null;
};

var monotonicCounters = {};
var monotonicCountersCount = 0;

// Override this method if you want a different mock implementation of monotonic counters.
SecureWorker._createMonotonicCounter = function _createMonotonicCounter(context) {
  if (monotonicCountersCount >= 256) throw new context.Error("Monotonic counter limit reached.");

  monotonicCountersCount++;
  var counterId = uuid.v4({}, new context.ArrayBuffer());
  var counterIdStr = uuid.unparse(counterId);
  monotonicCounters[counterIdStr] = 0;
  return new context.Object({
    uuid: counterId,
    value: monotonicCounters[counterIdStr]
  });
};

// Override this method if you want a different mock implementation of monotonic counters.
SecureWorker._destroyMonotonicCounter = function _destroyMonotonicCounter(context, counterId) {
  var counterIdStr = uuid.unparse(counterId);
  if (!monotonicCounters.hasOwnProperty(counterIdStr)) throw new context.Error("Unknown monotonic counter.");

  delete monotonicCounters[counterIdStr];
  monotonicCountersCount--;
};

// Override this method if you want a different mock implementation of monotonic counters.
SecureWorker._readMonotonicCounter = function _readMonotonicCounter(context, counterId) {
  var counterIdStr = uuid.unparse(counterId);
  if (!monotonicCounters.hasOwnProperty(counterIdStr)) throw new context.Error("Unknown monotonic counter.");

  return monotonicCounters[counterIdStr];
};

// Override this method if you want a different mock implementation of monotonic counters.
SecureWorker._incrementMonotonicCounter = function _incrementMonotonicCounter(context, counterId) {
  var counterIdStr = uuid.unparse(counterId);
  if (!monotonicCounters.hasOwnProperty(counterIdStr)) throw new context.Error("Unknown monotonic counter.");

  return ++monotonicCounters[counterIdStr];
};

var timeSourceNonce = crypto.randomBytes(32);
timeSourceNonce = new Uint8Array(timeSourceNonce.buffer.slice(timeSourceNonce.byteOffset, timeSourceNonce.byteOffset + timeSourceNonce.byteLength));

// Override this method if you want a different mock implementation of trusted time.
SecureWorker._getTrustedTime = function _getTrustedTime(context) {
  var currentTime = new Uint8Array(new Uint64LE(Math.floor(new Date() / 1000)).toArrayBuffer());

  // Copying to ArrayBuffers from context.
  var contextCurrentTime = new context.ArrayBuffer(currentTime.byteLength);
  var contextTimeSourceNone = new context.ArrayBuffer(timeSourceNonce.byteLength);
  new context.Uint8Array(contextCurrentTime).set(currentTime);
  new context.Uint8Array(contextTimeSourceNone).set(timeSourceNonce);

  return new context.Object({
    currentTime: contextCurrentTime,
    timeSourceNonce: contextTimeSourceNone
  });
};

SecureWorker._getReport = function _getReport(context, reportData, targetInfo) {
  // Using == on purpose.
  if (!((reportData == null) || (reportData instanceof context.ArrayBuffer && reportData.byteLength === 64))) throw new context.Error("Invalid report data.");

  if (!((targetInfo == null) || (targetInfo instanceof context.ArrayBuffer && targetInfo.byteLength === 512))) throw new context.Error("Invalid target info.");

  // report = sgx_report_body_t (384 B) + sgx_key_id_t (32 B) + sgx_mac_t (16 B)
  // sgx_report_body_t = sgx_cpu_svn_t (16 B) + sgx_misc_select_t (4 B) + uint8_t[28] + sgx_attributes_t (16 B) + sgx_measurement_t (32 B) + uint8_t[32] + sgx_measurement_t (32 B) + uint8_t[96] + sgx_prod_id_t (2 B) + sgx_isv_svn_t (2 B) + uint8_t[60] + sgx_report_data_t (64 B)
  // length of the report in bytes: 320 B + 64 B (report data) + 48 B (key + mac)

  var report = new context.ArrayBuffer(320 + 64 + 48);

  if (reportData) {
    var view = new context.Uint8Array(report);
    view.set(new context.Uint8Array(reportData), 320);
  }

  return report;
};

// Class methods for this mock implementation which can and should be overridden by the user of the library.
SecureWorker._resolveContentKey = function _resolveContentKey(enclaveName, contentKey) {
  throw new Error("Not implemented.");
};

module.exports = SecureWorker;
