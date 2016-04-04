Promise._setImmediateFn(_dukEnclaveNative.nextTick);

var console = {
	log: function () {
		_dukEnclaveNative.debug(Array.prototype.join.call(arguments, ' '));
	},
};

var F = (function () {
	var F = {};
	var alreadyImportedScripts = [];
	F.importScripts = function (/* args */) {
		for (var i = 0; i < arguments.length; i++) {
			var contentKey = arguments[i];
			if (alreadyImportedScripts.indexOf(contentKey) !== -1) continue;
			alreadyImportedScripts.push(contentKey);
			try {
				_dukEnclaveNative.importScript(contentKey);
			} catch (e) {
				_dukEnclaveNative.debug(e.stack || e);
			}
		}
	};
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
			} catch (e) {
				_dukEnclaveNative.debug(e.stack || e);
			}
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
		return new Uint8Array(Duktape.Buffer(src), src.byteOffset, src.byteLength);
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
	var EC256_SPKI_PADDING = new Uint8Array([0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42]);
	var EC256_PKCS8_PADDING = new Uint8Array([0x30, 0x81, 0x87, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x04, 0x6D, 0x30, 0x6B, 0x02, 0x01, 0x01, 0x04, 0x20]);
	var EC256_PKCS8_PADDING_BARE = new Uint8Array([0x30, 0x41, 0x02, 0x01, 0x00, 0x30, 0x13, 0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x04, 0x27, 0x30, 0x25, 0x02, 0x01, 0x01, 0x04, 0x20]);
	var removePadding = function (src, padding, length) {
		for (var i = 0; i < padding.length; i++) {
			if (src[i] !== padding[i]) return null;
		}
		return src.subarray(padding.length, padding.length + length);
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
					return Promise.resolve(_dukEnclaveNative.ecdhDeriveBits(algorithm.public.raw, baseKey.raw));
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
					result.raw = keyData.slice().buffer;
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
					result = key.raw.slice();
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
			},
		},
	};
	return crypto;
})();
