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
                        return Promise.resolve(_dukEnclaveNative.ecdsaSign(key.raw, data));
                    default:
                        throw new Error('algorithm ' + algorithm.name + ' not supported');
                }
            },
            verify: function (algorithm, key, signature, data) {
                algorithm = normalizeAlgorithmShallow(algorithm);
                switch (algorithm.name) {
                    case 'ECDSA':
                        algorithm.hash = normalizeAlgorithmShallow(algorithm.hash);
                        if (algorithm.hash.name !== 'SHA-256') throw new Error('hash ' + algorithm.hash.name + ' not supported');
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
                    type: 'secret',
                    extractable: true,
                    algorithm: algorithm,
                    usages: keyUsages,
                    raw: keyData,
                };
                switch (algorithm.name) {
                    case 'ECDH':
                    case 'ECDSA':
                        var keyLength = keyData.byteLength || keyData.length;
                        if (keyLength === 64) {
                            result.type = 'public';
                        } else if (keyLength === 32) {
                            result.type = 'private';
                        } else {
                            throw new Error('unrecognized keyData length');
                        }
                }
                return Promise.resolve(result);
            },
            exportKey: function (format, key) {
                if (format !== 'raw') throw new Error('format ' + format + ' not supported');
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
