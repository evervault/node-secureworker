#!/usr/bin/env node

var child_process = require('child_process');
var fs = require('fs');
var path = require('path');
var crypto = require('crypto');
var _ = require('underscore');

process.chdir(__dirname);

child_process.execFileSync('../bin/secureworker-create', [
  'test.js',
  'test-commands.js'
], {
  stdio: 'inherit',
  encoding: 'utf8'
});

var MockSecureWorker = require('../lib/mock.js');

MockSecureWorker._resolveContentKey = function _resolveContentKey(enclaveName, contentKey) {
  return fs.readFileSync(path.join(__dirname, contentKey), 'utf8');
};

var RealSecureWorker = require('../lib/real.js');

var ALL_TESTS = 6;

var testsPassed = 0;
var testsFailed = 0;

var timeout = setTimeout(function () {
  console.error(`Timeout. ${testsPassed} tests passed, ${testsFailed} tests failed, ${2 * ALL_TESTS - testsPassed - testsFailed} tests pending.`);
  process.exit(1);
}, 5 * 1000); // ms

var reportTests = function reportTests() {
  if (testsPassed === 2 * ALL_TESTS && testsFailed === 0) {
    console.log(`All ${testsPassed} tests passed.`);
    clearTimeout(timeout);
    process.exit(0);
  }
  else if (testsPassed + testsFailed >= 2 * ALL_TESTS) {
    console.error(`${testsPassed} tests passed, ${testsFailed} tests failed.`);
    clearTimeout(timeout);
    process.exit(1);
  }
};

var bufferToTime = function bufferToTime(buffer) {
  var view = new DataView(new Uint8Array(buffer).buffer);

  return view.getUint32(0, true) + view.getUint32(4, true) * Math.pow(2, 32);
};

var mockWorker = new MockSecureWorker('enclave.so', 'test.js');

var realWorker = new RealSecureWorker('enclave.so', 'test.js');

[{name: "Mock", worker: mockWorker}, {name: "Real", worker: realWorker}].forEach(function (type) {
  type.worker.onMessage(function (message) {
    if (message.success === true) {
      console.log(type.name + " test passed: " + message.name);
      testsPassed++;
      reportTests();
    }
    else if (message.success === false) {
      console.error(type.name +  " test failed: " + message.name);
      testsFailed++;
      reportTests();
    }
  });

  var previousTime = null;
  var timeSourceNonce = null;

  type.worker.onMessage(function listener(message) {
    if (message.command !== 'time') return;

    if (!previousTime && !timeSourceNonce) {
      previousTime = Buffer.from(message.currentTime, 'base64');
      timeSourceNonce = Buffer.from(message.timeSourceNonce, 'base64');

      setTimeout(function () {
         type.worker.postMessage({command: 'time'});
      }, 1000); // ms
    }
    else {
      type.worker.removeOnMessage(listener);

      var newTime = Buffer.from(message.currentTime, 'base64');

      if (!Buffer.from(message.timeSourceNonce, 'base64').equals(timeSourceNonce)) {
        console.error("Time nonce changed.");
        testsFailed++;
        reportTests();
        return;
      }

      if (bufferToTime(newTime) === bufferToTime(previousTime) + 1) {
        console.log(type.name +  " test passed: trusted time");
        testsPassed++;
        reportTests();
      }
      else {
        console.error(type.name +  " test failed: trusted time");
        testsFailed++;
        reportTests();
      }
    }
  });

  type.worker.postMessage({command: 'time'});

  var data = crypto.randomBytes(64);

  type.worker.onMessage(function listener(message) {
    if (message.command !== 'report') return;
    type.worker.removeOnMessage(listener);

    var report = new Uint8Array(Buffer.from(message.report, 'base64').values()).buffer;

    var reportData = type.worker.constructor.getReportData(report);

    if (_.isEqual(new Uint8Array(data), new Uint8Array(reportData))) {
      console.log(type.name +  " test passed: report data");
      testsPassed++;
    }
    else {
      console.error(type.name +  " test failed: report data");
      testsFailed++;
    }

    try {
      var quote = type.worker.constructor.getQuote(report, false, new ArrayBuffer(16));

      var quoteData = type.worker.constructor.getQuoteData(quote);

      if (_.isEqual(new Uint8Array(data), new Uint8Array(quoteData))) {
        console.log(type.name +  " test passed: quote data");
        testsPassed++;
      }
      else {
        console.error(type.name +  " test failed: quote data");
        testsFailed++;
      }
    }
    catch (error) {
      console.error(error);
      console.error(type.name +  " test failed: quote data");
      testsFailed++;
    }

    reportTests();
  });

  type.worker.postMessage({
    command: 'report',
    reportData: data.toString('base64')
  });
});

// TODO: Test remote attestation and validation of remote attestation.