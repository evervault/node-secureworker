var events = require('events');
var util = require('util');

var MessagePort = function MessagePort() {
  var self = this;

  events.EventEmitter.call(self);

  // Increase max listeners.
  self.setMaxListeners(100);

  self._started = false;
  self._queuedBeforeStarting = [];
};

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

module.exports = MessagePort;
