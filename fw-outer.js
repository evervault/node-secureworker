var events = require('events');
var m = require('m');
exports.postMessage = function (message) {
	setImmediate(function () {
		m._native.emitMessage(message);
	});
};
var messageEmitter = new events.EventEmitter();
exports.onMessage = function (callback) {
	messageEmitter.addListener('message', callback);
};
m._handlers.postMessage = function (message) {
	setImmediate(function () {
		messageEmitter.emit('message', message);
	});
};