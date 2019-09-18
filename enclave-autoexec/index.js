(function () {
  (function (root) {
    root.self = root;
    root.global = root;

     // So that promise-polyfill does not crash.
    root.setTimeout = null;
    // Sets Promise on root.
    var Promise = require('promise-polyfill');
    delete root.setTimeout;

    Promise._immediateFn = _dukEnclaveNative.nextTick;

    root.nextTick = _dukEnclaveNative.nextTick;
    root.setImmediate = _dukEnclaveNative.nextTick;

    root.Promise = Promise;

    root.console = {
      log: function () {
        _dukEnclaveNative.debug(Array.prototype.join.call(arguments, ' '));
      },

      warn: function () {
        _dukEnclaveNative.debug(Array.prototype.join.call(arguments, ' '));
      },

      error: function () {
        _dukEnclaveNative.debug(Array.prototype.join.call(arguments, ' '));
      }
    };

    var alreadyImportedScripts = [];
    var beingImportedScripts = [];
    var messageHandlers = [];

    root.SecureWorker = {
      ready: Promise.resolve(),

      getName: function getName() {
        throw new Error("Not implemented.");
      },

      onMessage: function onMessage(listener) {
        messageHandlers.push(listener);

        return listener;
      },

      removeOnMessage: function removeOnMessage(listener) {
        var index = messageHandlers.indexOf(listener);
        if (index === -1) return;
        messageHandlers.splice(index, 1);
      },

      postMessage: function postMessage(message) {
        var marshalledMessage = JSON.stringify(message);
        _dukEnclaveNative.postMessage(marshalledMessage);
      },

      close: function close() {
        throw new Error("Not implemented.");
      },

      importScripts: function (/* args */) {
        for (var i = 0; i < arguments.length; i++) {
          var contentKey = arguments[i];

          if (alreadyImportedScripts.indexOf(contentKey) !== -1) continue;
          if (beingImportedScripts.indexOf(contentKey) !== -1) continue;
          beingImportedScripts.push(contentKey);

          try {
            _dukEnclaveNative.importScript(contentKey);

            // Successfully imported.
            alreadyImportedScripts.push(contentKey);
          }
          catch (error) {
            _dukEnclaveNative.debug(error.stack || error);
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
        // Returns an object {uuid:ArrayBuffer, value:Number}.
        create: function create() {
          return _dukEnclaveNative.createMonotonicCounter();
        },

        destroy: function destroy(counterId) {
          _dukEnclaveNative.destroyMonotonicCounter(counterId);
        },

        // Returns the number.
        read: function read(counterId) {
          return _dukEnclaveNative.readMonotonicCounter(counterId);
        },

        // Returns the number.
        increment: function increment(counterId) {
          return _dukEnclaveNative.incrementMonotonicCounter(counterId);
        }
      },

      // Returns an object {currentTime:ArrayBuffer, timeSourceNonce:ArrayBuffer}.
      getTrustedTime: function getTrustedTime() {
        return _dukEnclaveNative.getTime();
      },

      // Returns the report as ArrayBuffer. reportData is 64 bytes of extra information, targetInfo is 512 bytes, ArrayBuffers. Both optional.
      getReport: function getReport(reportData, targetInfo) {
        // Only if it is undefined, we fetch target info ourselves. If it is null, we leave it null.
        if (targetInfo === undefined) {
          var initQuote = _dukEnclaveNative.initQuote();
          targetInfo = initQuote.targetInfo;
        }
        
        return _dukEnclaveNative.getReport(reportData, targetInfo);
      },

      // Seals an ArrayBuffer along with an additional buffer of *unencrypted* text
      sealData: function sealData(additionalData, data) {
        if (!additionalData || additionalData.length === 0) additionalData = null;

        return _dukEnclaveNative.sealData(additionalData, data);
      },

      // Returns an object from a sealed ArrayBuffer {data: ArrayBuffer, additionalData: ArrayBuffer}
      unsealData: function unsealData(data) {
        return _dukEnclaveNative.unsealData(data);
      }
    };

    _dukEnclaveHandlers.emitMessage = function (marshalledMessage) {
      var message = JSON.parse(marshalledMessage);
      var messageHandlersSnapshot = messageHandlers.slice();
      for (var i = 0; i < messageHandlersSnapshot.length; i++) {
        try {
          messageHandlersSnapshot[i](message);
        }
        catch (e) {
          _dukEnclaveNative.debug(e.stack || e);
        }
      }
    };

    root.crypto = (function () {
      var normalizeAlgorithmShallow = function (alg) {
        if (typeof alg === 'string') return { name: alg };
        return alg;
      };

      var normalizeBufferSource = function (src) {
        return new Uint8Array(Buffer(src), src.byteOffset, src.byteLength);
      };

      var equals128 = function (a, b) {
        if (a.length !== 16 || b.length !== 16) return false;
        for (var i = 0; i < 16; i++) {
          if (a[i] !== b[i]) return false;
        }
        return true;
      };

      var reverse256 = function (dst, src) {
        for (var i = 0; i < 32; i++) {
          dst[i] = src[31 - i];
        }
      };
      var EC256_SPKI_PADDING = new Uint8Array([0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04]);
      var EC256_PKCS8_PADDING = new Uint8Array([0x30, 0x81, 0x87, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x04, 0x6D, 0x30, 0x6B, 0x02, 0x01, 0x01, 0x04, 0x20]);
      var EC256_PKCS8_PADDING_BARE = new Uint8Array([0x30, 0x41, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x04, 0x27, 0x30, 0x25, 0x02, 0x01, 0x01, 0x04, 0x20]);
      var removePadding = function (src, padding, length) {
        for (var i = 0; i < padding.length; i++) {
          if (src[i] !== padding[i]) return null;
        }
        return src.subarray(padding.length, padding.length + length);
      };

      return {
        getRandomValues: function (array) {
          return _dukEnclaveNative.getRandomValues(array);
        },

        subtle: {
          encrypt: function (algorithm, key, data) {
            algorithm = normalizeAlgorithmShallow(algorithm);
            switch (algorithm.name) {
            case 'AES-GCM':
              if (!algorithm.tagLength) algorithm.tagLength = 128;
              if (algorithm.tagLength !== 128) throw new Error('tagLength ' + algorithm.tagLength + ' not supported');
              return Promise.resolve(_dukEnclaveNative.aesgcmEncrypt(algorithm.iv, algorithm.additionalData, key.raw, data));
            case 'AES-CTR':
              return Promise.resolve(_dukEnclaveNative.aesctrEncrypt(algorithm.counter, algorithm.length, key.raw, data));
            default:
              throw new Error('algorithm ' + algorithm.name + ' not supported');
            }
          },

          decrypt: function (algorithm, key, data) {
            algorithm = normalizeAlgorithmShallow(algorithm);
            switch (algorithm.name) {
            case 'AES-GCM':
              if (!algorithm.tagLength) algorithm.tagLength = 128;
              if (algorithm.tagLength !== 128) throw new Error('tagLength ' + algorithm.tagLength + ' not supported');
              return Promise.resolve(_dukEnclaveNative.aesgcmDecrypt(algorithm.iv, algorithm.additionalData, key.raw, data));
            case 'AES-CTR':
              return Promise.resolve(_dukEnclaveNative.aesctrDecrypt(algorithm.counter, algorithm.length, key.raw, data));
            default:
              throw new Error('algorithm ' + algorithm.name + ' not supported');
            }
          },

          sign: function (algorithm, key, data) {
            algorithm = normalizeAlgorithmShallow(algorithm);
            switch (algorithm.name) {
            case 'AES-CMAC':
              if (algorithm.length !== 128) throw new Error('length ' + algorithm.length + ' not supported');
              return Promise.resolve(_dukEnclaveNative.aescmacSign(key.raw, data));
            case 'ECDSA':
              algorithm.hash = normalizeAlgorithmShallow(algorithm.hash);
              if (algorithm.hash.name !== 'SHA-256') throw new Error('hash ' + algorithm.hash.name + ' not supported');
              var signatureLE = _dukEnclaveNative.ecdsaSign(key.raw, data);
              var signature = new ArrayBuffer(64);
              reverse256(new Uint8Array(signature, 0, 32), new Uint8Array(signatureLE, 0, 32));
              reverse256(new Uint8Array(signature, 32, 32), new Uint8Array(signatureLE, 32, 32));
              return Promise.resolve(signature);
            default:
              throw new Error('algorithm ' + algorithm.name + ' not supported');
            }
          },

          verify: function (algorithm, key, signature, data) {
            algorithm = normalizeAlgorithmShallow(algorithm);
            signature = normalizeBufferSource(signature);
            switch (algorithm.name) {
            case 'AES-CMAC':
              if (algorithm.length !== 128) throw new Error('length ' + algorithm.length + ' not supported');
              var expectedSignature = _dukEnclaveNative.aescmacSign(key.raw, data);
              return Promise.resolve(equals128(signature, new Uint8Array(expectedSignature)));
            case 'ECDSA':
              algorithm.hash = normalizeAlgorithmShallow(algorithm.hash);
              if (algorithm.hash.name !== 'SHA-256') throw new Error('hash ' + algorithm.hash.name + ' not supported');
              var signatureLE = new ArrayBuffer(64);
              reverse256(new Uint8Array(signatureLE, 0, 32), signature.subarray(0, 32));
              reverse256(new Uint8Array(signatureLE, 32, 32), signature.subarray(32, 64));
              return Promise.resolve(_dukEnclaveNative.ecdsaVerify(key.raw, signatureLE, data));
            default:
              throw new Error('algorithm ' + algorithm.name + ' not supported');
            }
          },

          digest: function (algorithm, data) {
            algorithm = normalizeAlgorithmShallow(algorithm);
            switch (algorithm.name) {
            case 'SHA-256':
              return Promise.resolve(_dukEnclaveNative.sha256Digest(data));
            default:
              throw new Error('algorithm ' + algorithm.name + ' not supported');
            }
          },

          generateKey: function (algorithm, extractable, keyUsages) {
            algorithm = normalizeAlgorithmShallow(algorithm);
            switch (algorithm.name) {
            case 'ECDSA':
            case 'ECDH':
              if (algorithm.namedCurve !== 'P-256') throw new Error('namedCurve ' + algorithm.namedCurve + ' not supported');
              var algorithmOut = {
                name: algorithm.name,
                namedCurve: algorithm.namedCurve,
              };
              var rawKeyPair = _dukEnclaveNative.ecGenerateKey();
              return Promise.resolve({
                publicKey: {
                  type: 'public',
                  extractable: true,
                  algorithm: algorithmOut,
                  usages: ['verify'],
                  raw: rawKeyPair.publicKey,
                },
                privateKey: {
                  type: 'private',
                  extractable: true,
                  algorithm: algorithmOut,
                  usages: ['sign', 'deriveKey', 'deriveBits'],
                  raw: rawKeyPair.privateKey,
                },
              });
            default:
              throw new Error('algorithm ' + algorithm.name + ' not supported');
            }
          },

          deriveKey: function (algorithm, baseKey, derivedKeyType, extractable, keyUsage) {
            throw new Error('deriveKey not supported');
          },

          deriveBits: function (algorithm, baseKey, length) {
            algorithm = normalizeAlgorithmShallow(algorithm);
            switch (algorithm.name) {
            case 'ECDH':
              if (length !== 256) throw new Error('length ' + length + ' not supported');
              var bitsLE = _dukEnclaveNative.ecdhDeriveBits(algorithm.public.raw, baseKey.raw);
              var bits = new ArrayBuffer(32);
              reverse256(new Uint8Array(bits, 0, 32), new Uint8Array(bitsLE, 0, 32));
              return Promise.resolve(bits);
            default:
              throw new Error('algorithm ' + algorithm.name + ' not supported');
            }
          },

          importKey: function (format, keyData, algorithm, extractable, keyUsages) {
            algorithm = normalizeAlgorithmShallow(algorithm);
            keyData = normalizeBufferSource(keyData);
            var result = {
              type: null,
              extractable: true,
              algorithm: algorithm,
              usages: keyUsages,
              raw: null,
            };
            switch (algorithm.name) {
            case 'AES-GCM':
            case 'AES-CTR':
            case 'AES-CMAC':
              if (format !== 'raw') throw new Error('format ' + format + ' not supported');
              if (keyData.length !== 16) throw new Error('wrong keyData length');
              result.type = 'secret';
              result.raw = new Uint8Array(keyData).buffer;
              break;
            case 'ECDH':
            case 'ECDSA':
              if (algorithm.namedCurve !== 'P-256') throw new Error('namedCurve ' + algorithm.namedCurve + ' not supported');
              // TODO: What with all the endianness conversion, we might as well add the DER stuff.
              switch (format) {
              case 'spki':
                keyData = removePadding(keyData, EC256_SPKI_PADDING, 64);
                if (!keyData) throw new Error('could not parse ' + format);
                // Fall through
              case 'raw-public-uncompressed-be':
                if (keyData.length !== 64) throw new Error('wrong keyData length');
                result.type = 'public';
                result.raw = new ArrayBuffer(64);
                reverse256(new Uint8Array(result.raw, 0, 32), keyData.subarray(0, 32));
                reverse256(new Uint8Array(result.raw, 32, 32), keyData.subarray(32, 64));
                break;
              case 'pkcs8':
                // Caveat: We don't support all configurations of optional
                // fields in ECPrivateKey.
                // http://tools.ietf.org/html/rfc5915#section-3
                keyData = removePadding(keyData, EC256_PKCS8_PADDING, 32) ||
                  removePadding(keyData, EC256_PKCS8_PADDING_BARE, 32);
                if (!keyData) throw new Error('could not parse ' + format);
                // Fall through
              case 'raw-private-be':
                if (keyData.length !== 32) throw new Error('wrong keyData length');
                result.type = 'private';
                result.raw = new ArrayBuffer(32);
                reverse256(new Uint8Array(result.raw, 0, 32), keyData.subarray(0, 32));
                break;
              default:
                throw new Error('format ' + format + ' not supported');
              }
              break;
            default:
              throw new Error('algorithm ' + algorithm.name + ' not supported');
            }
            return Promise.resolve(result);
          },

          exportKey: function (format, key) {
            var result;
            switch (key.algorithm.name) {
            case 'AES-GCM':
            case 'AES-CTR':
            case 'AES-CMAC':
              if (format !== 'raw') throw new Error('format ' + format + ' not supported');
              result = new Uint8Array(new Uint8Array(key.raw)).buffer;
              break;
            case 'ECDH':
            case 'ECDSA':
              switch (format) {
              case 'spki':
                if (key.type !== 'public') throw new Error('wrong key type');
                result = new ArrayBuffer(EC256_SPKI_PADDING.length + 64);
                new Uint8Array(result).set(EC256_SPKI_PADDING);
                reverse256(new Uint8Array(result, EC256_SPKI_PADDING.length, 32), new Uint8Array(key.raw, 0, 32));
                reverse256(new Uint8Array(result, EC256_SPKI_PADDING.length + 32, 32), new Uint8Array(key.raw, 32, 32));
                break;
              case 'raw-public-uncompressed-be':
                if (key.type !== 'public') throw new Error('wrong key type');
                result = new ArrayBuffer(64);
                reverse256(new Uint8Array(result, 0, 32), new Uint8Array(key.raw, 0, 32));
                reverse256(new Uint8Array(result, 32, 32), new Uint8Array(key.raw, 32, 32));
                break;
              case 'pkcs8':
                if (key.type !== 'private') throw new Error('wrong key type');
                result = new ArrayBuffer(EC256_PKCS8_PADDING_BARE.length + 32);
                new Uint8Array(result).set(EC256_PKCS8_PADDING_BARE);
                reverse256(new Uint8Array(result, EC256_PKCS8_PADDING_BARE.length, 32), new Uint8Array(key.raw, 0, 32));
                break;
              case 'raw-private-be':
                if (key.type !== 'private') throw new Error('wrong key type');
                result = new ArrayBuffer(32);
                reverse256(new Uint8Array(result, 0, 32), new Uint8Array(key.raw, 0, 32));
                break;
              default:
                throw new Error('format ' + format + ' not supported');
              }
              break;
            default:
              throw new Error('algorithm ' + key.algorithm.name + ' not supported');
            }
            return Promise.resolve(result);
          },

          wrapKey: function (format, key, wrappingKey, wrapAlgorithm) {
            throw new Error('wrapKey not supported');
          },

          unwrapKey: function (format, key, wrappingKey, wrapAlgorithm, unwrappedKeyAlgorithm, extractable, keyUsages) {
            throw new Error('unwrapKey not supported');
          }
        }
      };
    })();
  })(this);
})();
