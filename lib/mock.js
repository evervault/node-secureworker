var vm = require('vm');
var events = require('events');
var crypto = require('crypto');
var util = require('util');
var Promise = require('promise-polyfill');
var subtle = require('subtle');

function MessagePort() {
  var self = this;

  events.EventEmitter.call(self);

  self._started = false;
  self._queuedBeforeStarting = [];
}

util.inherits(MessagePort, events.EventEmitter);

MessagePort.prototype.start = function start() {
  var self = this;

  if (self._started) return;
  self._started = true;

  var eventArguments;
  while (eventArguments = self._queuedBeforeStarting.shift()) {
    // Because _started is true, this will now emit events properly.
    self.emit.apply(self, eventArguments);
  }
};

MessagePort.prototype.emit = function emit(/* args */) {
  var self = this;

  if (!self._started) {
    self._queuedBeforeStarting.push(arguments);
    return;
  }

  // Calling super.
  return events.EventEmitter.prototype.emit.apply(self, arguments);
};

var SecureWorker = function SecureWorker(contentKey) {
  var self = this;

  if (!(self instanceof SecureWorker)) {
    return new SecureWorker.apply(null, arguments);
  }

  self._eventsFromOutside = new MessagePort();
  self._eventsFromInside = new MessagePort();

  var sandbox = this.constructor._sandboxContext(self, contentKey);

  self._context = vm.createContext(sandbox);

  sandbox.F.importScripts(contentKey);

  return self;
};

SecureWorker.prototype.onMessage = function onMessage(listener) {
  var self = this;

  self._eventsFromInside.addListener('message', listener);
  self._eventsFromInside.start();

  return listener;
};

SecureWorker.prototype.removeOnMessage = function removeOnMessage(listener) {
  var self = this;

  self._eventsFromInside.removeListener('message', listener);
};

SecureWorker.prototype.postMessage = function postMessage(message) {
  var self = this;

  // We want to simulate asynchronous messaging.
  setImmediate(function () {
    self._eventsFromOutside.emit('message', message);
  });
};

SecureWorker.prototype.terminate = function terminate() {
  var self = this;

  // TODO: Is there a way to implement this using "vm"? If there are no timers it should be possible?
  // A noop in this mock implementation.
};

// Class methods for this mock implementation which should be overridden by the user of the package.
SecureWorker._resolveContentKey = function _resolveContentKey() {
  throw new Error("Not implemented.");
};

SecureWorker._createMonotonicCounter = function _createMonotonicCounter() {
  throw new Error("Not implemented.");
};

SecureWorker._destroyMonotonicCounter = function _destroyMonotonicCounter() {
  throw new Error("Not implemented.");
};

SecureWorker._readMonotonicCounter = function _readMonotonicCounter() {
  throw new Error("Not implemented.");
};

SecureWorker._incrementMonotonicCounter = function _incrementMonotonicCounter() {
  throw new Error("Not implemented.");
};

// Class method for this mock implementation to allow specifying sandbox context.
SecureWorker._sandboxContext = function _sandboxContext(secureWorker, contentKey) {
  var sandbox = {
    // Our internal trusted API.
    F: {
      ready: Promise.resolve(),

      getName: function getName() {
        return contentKey;
      },

      // Callbacks are called only after F.ready resolves.
      onMessage: function onMessage(listener) {
        secureWorker._eventsFromOutside.addListener('message', listener);

        sandbox.F.ready.then(function () {
          secureWorker._eventsFromOutside.start();
        });

        return listener;
      },

      removeOnMessage: function removeOnMessage(listener) {
        secureWorker._eventsFromOutside.removeListener('message', listener);
      },

      postMessage: function postMessage(message) {
        // We want to simulate asynchronous messaging.
        setImmediate(function () {
          secureWorker._eventsFromInside.emit('message', message);
        });
      },

      close: function close() {
        secureWorker.terminate();
      },

      importScripts: function importScripts(/* args */) {
        for (var i = 0; i < arguments.length; i++) {
          var contentKey = arguments[i];
          var code = SecureWorker._resolveContentKey(contentKey);

          vm.runInContext(code, secureWorker._context, {
            filename: contentKey,
            displayErrors: true
          });
        }
      }
    },

    Promise: Promise,

    crypto: {
      subtle: subtle,

      // Based on: https://github.com/KenanY/get-random-values
      getRandomValues: function getRandomValues(typedArray) {
        if (typedArray.byteLength > 65536) {
          var error = new Error();
          error.code = 22;
          error.message = 'Failed to execute \'getRandomValues\' on \'Crypto\': The ' +
            'ArrayBufferView\'s byte length (' + typedArray.byteLength  + ') exceeds the ' +
            'number of bytes of entropy available via this API (65536).';
          error.name = 'QuotaExceededError';
          throw error;
        }
        var bytes = crypto.randomBytes(typedArray.byteLength);
        typedArray.set(bytes);
        return typedArray;
      },

      monotonicCounters: {
        create: function create() {
          return SecureWorker._createMonotonicCounter();
        },

        destroy: function destroy(counterId) {
          return SecureWorker._destroyMonotonicCounter(counterId);
        },

        read: function read(counterId) {
          return SecureWorker._readMonotonicCounter(counterId);
        },

        increment: function increment(counterId) {
          return SecureWorker._incrementMonotonicCounter(counterId);
        }
      }
    }
  };

  sandbox.self = sandbox;

  return sandbox;
};

module.exports = SecureWorker;
