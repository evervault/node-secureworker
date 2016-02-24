_dukEnclaveNative.postMessage('"hello from framework"');

Promise._setImmediateFn(_dukEnclaveNative.nextTick);

var console = {
    log: function () {
        _dukEnclaveNative.debug(Array.prototype.join.call(arguments, ' '));
    },
};

var F = (function () {
    var F = {};
    F.postMessage = function (message) {
        var marshalledMessage = JSON.stringify(message);
        _dukEnclaveNative.postMessage(marshalledMessage);
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
    _dukEnclaveHandlers.emitMessage = function (marshalledMessage) {
        var message = JSON.parse(marshalledMessage);
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

var crypto = (function () {
    var normalizeAlgorithmShallow = function (alg) {
        if (typeof alg === 'string') return { name: alg };
        return alg;
    };
    var normalizeBufferSource = function (src) {
        if (typeof src === 'buffer') {
            return new Uint8Array(src);
        } else {
            return new Uint8Array(Duktape.Buffer(src), src.byteOffset, src.byteLength);
        }
    };
    var equals128 = function (a, b) {
        if (a.byteLength !== 16 || b.byteLength !== 16) return false;
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
    var crypto = {
        getRandomValues: function (array) {
            throw new Error('getRandomValues not supported');
        },
        subtle: {
            encrypt: function (algorithm, key, data) {
                algorithm = normalizeAlgorithmShallow(algorithm);
                switch (algorithm.name) {
                    case 'AES-GCM':
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
                switch (algorithm.name) {
                    case 'AES-CMAC':
                        if (algorithm.length !== 128) throw new Error('length ' + algorithm.length + ' not supported');
                        var expectedSignature = _dukEnclaveNative.aescmacSign(key.raw, data);
                        return Promise.resolve(equals128(normalizeBufferSource(signature), new Uint8Array(expectedSignature)));
                    case 'ECDSA':
                        algorithm.hash = normalizeAlgorithmShallow(algorithm.hash);
                        if (algorithm.hash.name !== 'SHA-256') throw new Error('hash ' + algorithm.hash.name + ' not supported');
                        signature = normalizeBufferSource(signature);
                        var signatureLE = new ArrayBuffer(64);
                        reverse256(new Uint8Array(signatureLE, 0, 32), new Uint8Array(signature, 0, 32));
                        reverse256(new Uint8Array(signatureLE, 32, 32), new Uint8Array(signature, 32, 32));
                        return Promise.resolve(_dukEnclaveNative.ecdsaVerify(key.raw, signature, data));
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
                                raw: rawKeyPair.privateKeym
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
                        if (length !== 32) throw new Error('length ' + length + ' not supported');
                        return Promise.resolve(_dukEnclaveNative.ecdhDeriveBits(algorithm.public.raw, baseKey.raw));
                    default:
                        throw new Error('algorithm ' + algorithm.name + ' not supported');
                }
            },
            importKey: function (format, keyData, algorithm, extractable, keyUsages) {
                if (format !== 'raw') throw new Error('format ' + format + ' not supported');
                // Haaaaaaack. Assume it's a secret key, and guess based on the length for ECDH/ECDSA.
                algorithm = normalizeAlgorithmShallow(algorithm);
                var result = {
                    type: null,
                    extractable: true,
                    algorithm: algorithm,
                    usages: keyUsages,
                    raw: null,
                };
                // TODO: What with all the endianness conversion, we might as well add the DER stuff.
                switch (algorithm.name) {
                    case 'ECDH':
                    case 'ECDSA':
                        if (algorithm.namedCurve !== 'P-256') throw new Error('namedCurve ' + algorithm.namedCurve + ' not supported');
                        var keyLength = keyData.byteLength || keyData.length;
                        if (keyLength === 64) {
                            result.type = 'public';
                            result.raw = new ArrayBuffer(64);
                            reverse256(new Uint8Array(result.raw, 0, 32), new Uint8Array(keyData, 0, 32));
                            reverse256(new Uint8Array(result.raw, 32, 32), new Uint8Array(keyData, 32, 32));
                        } else if (keyLength === 32) {
                            result.type = 'private';
                            result.raw = new ArrayBuffer(32);
                            reverse256(new Uint8Array(result.raw, 0, 32), new Uint8Array(keyData, 0, 32));
                        } else {
                            throw new Error('unrecognized keyData length');
                        }
                        break;
                    default:
                        result.type = 'secret';
                        result.raw = keyData;
                }
                return Promise.resolve(result);
            },
            exportKey: function (format, key) {
                if (format !== 'raw') throw new Error('format ' + format + ' not supported');
                // TODO: Swap endianness.
                return Promise.resolve(key.raw);
            },
            wrapKey: function (format, key, wrappingKey, wrapAlgorithm) {
                throw new Error('wrapKey not supported');
            },
            unwrapKey: function (format, key, wrappingKey, wrapAlgorithm, unwrappedKeyAlgorithm, extractable, keyUsages) {
                throw new Error('unwrapKey not supported');
            },
        },
    };
    return crypto;
})();
