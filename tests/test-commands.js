// Duktape's Buffer does not support toString.
// See: https://github.com/svaarala/duktape/issues/1397
function toBase64(data) {
  if (typeof Duktape !== 'undefined') {
    return Duktape.enc('base64', new Buffer(data));
  }
  else {
    return Buffer.from(data).toString('base64');
  }
}

function fromBase64(string) {
  if (typeof Duktape !== 'undefined') {
    return new Uint8Array(Duktape.dec('base64', string)).buffer;
  }
  else {
    return new Uint8Array(Buffer.from(string, 'base64').values()).buffer;
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

SecureWorker.onMessage(function (message) {
  if (message.command !== 'report') return;

  SecureWorker.postMessage({
    command: 'report',
    report: toBase64(SecureWorker.getReport(message.reportData ? fromBase64(message.reportData) : null))
  })
});
