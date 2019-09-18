var report = function (name) {
  return function (reason) {
    console.log(name + ' failed:', reason.stack || reason);
    SecureWorker.postMessage({success: false, name: name});
  }
};

var passed = function (name) {
  return function () {
    SecureWorker.postMessage({success: true, name: name});
  };
};

var assertEqual = function (name, value1, value2) {
  if (value1 !== value2) {
    throw new Error(name + " not equal");
  }
};

var assertArraysEqual = function (name, array1, array2) {
  if (array1.length !== array2.length) {
    throw new Error(name + " not equal");
  }

  for (var i = 0; i < array1.length; i++) {
    if (array1[i] !== array2[i]) {
      throw new Error(name + " not equal");
    }
  }
};

var algorithm = {
  name: "ECDH",
  namedCurve: "P-256"
};
var uses = ["deriveKey", "deriveBits"];

SecureWorker.ready.then(function () {
  return Promise.all([
    crypto.subtle.generateKey(algorithm, true, uses),
    crypto.subtle.generateKey(algorithm, true, uses),
  ]);
}).then(function (generatedKeys) {
  return Promise.all([
    crypto.subtle.exportKey('pkcs8', generatedKeys[0].privateKey),
    crypto.subtle.exportKey('spki', generatedKeys[0].publicKey),
    crypto.subtle.exportKey('pkcs8', generatedKeys[1].privateKey),
    crypto.subtle.exportKey('spki', generatedKeys[1].publicKey)
  ]);
}).then(function (exportedKeys) {
  return Promise.all([
    crypto.subtle.importKey('pkcs8', exportedKeys[0], algorithm, false, uses),
    crypto.subtle.importKey('spki', exportedKeys[1], algorithm, false, uses),
    crypto.subtle.importKey('pkcs8', exportedKeys[2], algorithm, false, uses),
    crypto.subtle.importKey('spki', exportedKeys[3], algorithm, false, uses)
  ]);
}).then(function (importedKeys) {
  return Promise.all([
    crypto.subtle.deriveBits({name: 'ECDH', public: importedKeys[1]}, importedKeys[2], 256),
    crypto.subtle.deriveBits({name: 'ECDH', public: importedKeys[3]}, importedKeys[0], 256)
  ]);
}).then(function (derivedBits) {
  assertArraysEqual("derived bits", derivedBits[0], derivedBits[1]);

  return Promise.all([
    crypto.subtle.digest({name: 'SHA-256'}, derivedBits[0]),
    crypto.subtle.digest({name: 'SHA-256'}, derivedBits[1])
  ]);
}).then(function (hashedBits) {
  assertArraysEqual("hashed bits", hashedBits[0], hashedBits[1]);

  var keyData1 = new Uint8Array(hashedBits[0], 0, 16);
  var keyData2 = new Uint8Array(hashedBits[1], 0, 16);

  assertArraysEqual("generated keys", keyData1, keyData2);

  return crypto.subtle.importKey('raw', keyData1, {name: 'AES-GCM'}, false, ['encrypt', 'decrypt']);
}).then(function (key) {
  var iv = crypto.getRandomValues(new Uint8Array(12));

  var clearText = new Uint8Array(new Buffer("foobar"));

  return crypto.subtle.encrypt({name: 'AES-GCM', iv: iv}, key, clearText).then(function (cipherText) {
    return crypto.subtle.decrypt({name: 'AES-GCM', iv: iv}, key, cipherText);
  }).then(function (result) {
    result = new Uint8Array(result);

    assertArraysEqual("decrypted encrypted clear text", clearText, result);
  });
}).then(passed("crypto")).catch(report("crypto"));

SecureWorker.ready.then(function () {
  var counter = SecureWorker.monotonicCounters.create();

  try {
    var value = counter.value;

    assertEqual("monotonic counter read", SecureWorker.monotonicCounters.read(counter.uuid), value);

    assertEqual("monotonic counter increment", SecureWorker.monotonicCounters.increment(counter.uuid), value + 1);

    assertEqual("monotonic counter read after increment", SecureWorker.monotonicCounters.read(counter.uuid), value + 1);
  }
  finally {
    SecureWorker.monotonicCounters.destroy(counter.uuid);
  }
}).then(passed("monotonic counters")).catch(report("monotonic counters"));

SecureWorker.ready.then(function () {
  SecureWorker.importScripts('test-commands.js');
}).then(passed("import scripts")).catch(report("import scripts"));

// TODO: write tests for sealData and unsealData
