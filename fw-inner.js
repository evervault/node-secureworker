var F = (function () {
	var F = {
		_native: {},
		_handlers: {},
	};
	F.postMessage = function (message) {
		F._native.postMessage(message);
	};
	var messageHandlers = [];
	F.onMessage = function (callback) {
		messageHandlers.push(callback);
	};
	F.removeOnMessage = function (callback) {
		var index = messageHandlers.indexOf(callback);
		if (index === -1) return;
		messageHandlers.splice(index, 1);
	};
	F._handlers.emitMessage = function (message) {
		var messageHandlersSnapshot = messageHandlers.slice();
		for (var i = 0; i < messageHandlersSnapshot.length; i++) {
			try {
				messageHandlersSnapshot[i](message);
			} catch (e) { }
		}
	};
	F.ready = Promise.resolve();
	return F;
})();
