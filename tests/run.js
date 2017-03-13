#!/usr/bin/env node

var child_process = require('child_process');
var fs = require('fs');
var path = require('path');

process.chdir(__dirname);

child_process.execFileSync('../bin/build_enclave', [
  'test.js'
], {
  stdio: 'inherit',
  encoding: 'utf8'
});

var MockSecureWorker = require('../lib/mock.js');

MockSecureWorker._resolveContentKey = function _resolveContentKey(enclaveName, contentKey) {
  return fs.readFileSync(path.join(__dirname, contentKey), 'utf8');
};

var RealSecureWorker = require('../lib/real.js');

var ALL_TESTS = 2;

var timeout = setTimeout(function () {
  console.error("Timeout.");
  process.exit(1);
}, 5 * 1000); // ms

var testsPassed = 0;
var testsFailed = 0;

var report = function report() {
  if (testsPassed === ALL_TESTS && testsFailed === 0) {
    console.log(`All ${testsPassed} tests passed.`);
    clearTimeout(timeout);
    process.exit(0);
  }
  else if (testsPassed + testsFailed >= ALL_TESTS) {
    console.error(`${testsPassed} tests passed, ${testsFailed} tests failed.`);
    clearTimeout(timeout);
    process.exit(1);
  }
};

new MockSecureWorker('enclave.so', 'test.js').onMessage(function (message) {
  if (message.success) {
    console.log("Mock test passed.");
    testsPassed++;
  }
  else {
    console.error("Mock test failed.");
    testsFailed++;
  }

  report();
});

new RealSecureWorker('enclave.so', 'test.js').onMessage(function (message) {
  if (message.success) {
    console.log("Real test passed.");
    testsPassed++;
  }
  else {
    console.error("Real test failed.");
    testsFailed++;
  }

  report();
});

// TODO: Test getting report data, quote, quote data, remote attestation, and validation of remote attestation.