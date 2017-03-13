// Duktape's Buffer does not support toString.
// See: https://github.com/svaarala/duktape/issues/1397
function toBase64(data) {
  if (typeof Duktape !== 'undefined') {
    return Duktape.enc('base64', new Buffer(data));
  }
  else {
    return Buffer(data).toString('base64');
  }
}

SecureWorker.onMessage(function (message) {
  if (message.command !== 'time') return;

  var time = SecureWorker.getTrustedTime();

  SecureWorker.postMessage({
    command: 'time',
    currentTime: toBase64(time.currentTime),
    timeSourceNonce: toBase64(time.timeSourceNonce)
  })
});
