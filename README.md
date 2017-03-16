# SecureWorker

This NPM package allows you to run JavaScript inside a secure (trusted) environment (enclave) provided by
[Intel SGX](https://software.intel.com/en-us/sgx) technology on modern CPUs. When used properly,
even operating system or a cloud provider cannot access data, and observe and interfere with
operations inside an enclave.

This package executes JavaScript using the [Duktape](http://duktape.org/) JavaScript engine and provides
a web workers-like familiar environment. Cryptographic operations are exposed through a Web Crypto compatible API.
Promises are available as well. This allows you to write isometric code and use the same code on client, server,
and inside enclaves, and see secure workers as just another, secure and trusted, component in your JavaScript-based
architecture.

**Warning: The project is still in development. Feel free to contribute.**

## Installation

```
npm install secureworker --save
```

## Intel SGX requirements

Of course, to really use this package properly, you need [SGX hardware](https://github.com/ayeks/SGX-hardware).

But for development you have various options. Intel provides through its SDK an SGX simulator.
This package also fallbacks to mock mode of operation, where it uses [vm](https://nodejs.org/api/vm.html)
to simulate an isolated running environment, if it cannot load necessary binaries.
Of course, such execution does not give you added security, but it is good for experimentation.

If you would like to build enclaves, you need [Intel SGX SDK](https://software.intel.com/en-us/sgx-sdk/download).
To run them, you need [SGX PSW](https://github.com/01org/linux-sgx) (platform software, i.e., runtime).
To run them with SGX hardware, you need a [kernel driver](https://github.com/01org/linux-sgx-driver).

Alternativelly, you can use this [Docker image](https://hub.docker.com/r/tozd/sgx/) which has all SDK and PSW
already installed. (If you install SGX kernel driver on the host, you can even use it in production to run
your SGX enabled apps.)

**Warning: By default built enclaves are run in simulation and debug mode (or even mock mode). This does not provide any added security.**

[Read more about various types of compiling and executing enclaves](https://software.intel.com/en-us/blogs/2016/01/07/intel-sgx-debug-production-prelease-whats-the-difference).

## Bundling and building

Each enclave you build with this package (`.so` file) can contain multiple JavaScript files.
You can bundle each of them with your favorite tool. For example, for [browserify](http://browserify.org/)
the following options work well to create a `enclave-bundle.js` file from input `enclave-source.js`
file, bundling any imports into the bundle as well.

```
$ browserify --insert-global-vars __filename,__dirname --no-commondir -t [ babelify --presets [ es2015 ] ] enclave-source.js > enclave-bundle.js
```

Once you have files you want to build into an enclave (e.g., `enclave-bundle.js`, `tests.js`), you can use a tool
provided by this package to build an enclave file `enclave.so`:

```
$ secureworker-create --output enclave.so enclave-bundle.js tests.js
```

Scripts are identified inside an enclave using their basename.

Now, you can start your secure worker (enclave) using:

```javascript
const worker = new SecureWorker('enclave.so', 'enclave-bundle.js');
```

## Remote attestation 101

Running code inside a secure environment (enclave) nobody can inspect or tamper with is great, but not very helpful
if you cannot prove to others that you have really executed the code inside a specific enclave, or prove to
others that they are really communicating with the enclave, and not somebody who is pretending to be
that enclave.

To address this, Intel provides a service which can provide such proofs for you (now you have to trust
Intel, but you are already trusting them with [correctness of their CPUs](https://www.wired.com/2016/06/demonically-clever-backdoor-hides-inside-computer-chip/)).

The process to prove to others that something executed inside a specific *enclave*, simplified, can be as follows:

1. every *enclave* has a *measurement* which corresponds to its binary image (code);
   in our case this consists of code provided by this package to run JavaScript inside an
   *enclave*, and your scripts you built into the *enclave*
2. when *enclave* wants to produce such proof, it generates a *report* which binds
   a *measurement* with CPU identity and optional additional *report data* (often a *nonce* to
   prevent proof we are generating to be reused)
3. outside *enclave*, but on the same machine, a *report* is exchanged for a *quote* by a quoting *enclave* provided
   by Intel, and running on the machine as well
4. a *quote* is send to Intel's remote attestation service and if everything looks like it is coming
   from the SGX platform and their CPUs, it produces an *attestation*
5. one can verify offline this *attestation* using Intel's public key, furthermore, one should also
   verify that the *measurement* corresponds to the expected code, and probably check *report data*
   (especially if used as a *nonce*)

Alternatively, steps 4. and 5. can be done directly, online, by a peer who would
contact Intel's remote attestation service with a *quote* and obtain *attestation*.

On the other hand, sometimes one wants to know if they are communicating with a specific *enclave*. Often
so that they can provide it a secret nobody else should receive. In this case a process could be:

1. one should embed their public key in the *enclave*
2. one starts establishing a secure ephemeral channel with the *enclave*
3. *enclave* verifies the identity of a peer by using one's public key
4. one verifies the identity of the *enclave* by asking it to generate
   a *report* with a *nonce*
5. *report* is exchanged for a *quote*
6. one sends the *quote* Intel's remote attestation service to verify it, furthermore, one verifies the
   *measurement* and *nonce*
7. if everything matches, the secure channel can be trusted and one can be sure they communicate with
   the *enclave* directly

Intel SGX SDK provides a set of `sgx_ra_*` functions to help with the latter process, but this package
does not (yet) expose them. You can use Web Crypto API to instead establish a secure channel,
and have code around the enclave transmit messages between the peer and the enclave using `postMessage`.

## API (outside, untrusted)

### `new SecureWorker(enclaveName:String, contentKey:String) : SecureWorker`

Starts an enclave from file `enclaveName` and asynchronously runs the script under `contentKey` name.
It returns a worker instance.

### `SecureWorker.getReportData(report:ArrayBuffer) : ArrayBuffer`

Returns data provided when generating a `report`.

### `SecureWorker.getQuote(report:ArrayBuffer, linkable:Boolean, spid:ArrayBuffer, [revocationList:ArrayBuffer]) : ArrayBuffer`

Returns a quote based on the `report`. `spid` is SPID is an ID given to you by Intel when you register with the
Intel Attestation Service.

Quotes can be made `linkable` or anonymous. If it is linkable, it is bound to the CPU on which the report
was made. [More information](https://software.intel.com/en-us/blogs/2016/03/09/intel-sgx-epid-provisioning-and-attestation-services).

`revocationList` is a list of revoked CPU keys managed by Intel. If you do not provide one, a recent
one will be used automatically. If you pass `null`, none will be used at all.

**Automatic `revocationList` not yet implemented.**

### `SecureWorker.getQuoteData(quote:ArrayBuffer) : ArrayBuffer`

Returns data provided when generating a report, which is carried over to `quote`.

### `SecureWorker.getRemoteAttestation(quote:ArrayBuffer) : ArrayBuffer`

Returns a signed attestation for provided `quote`, if remote attestation succeeds.

**Not yet implemented.**

### `SecureWorker.validateRemoteAttestation(quote:ArrayBuffer, attestation:ArrayBuffer) : Boolean`

Validates that an `attestation`'s signature belongs to Intel's public key.

**Not yet implemented.**

### `SecureWorker.getSGXVersion() : Object`

Returns information about the SGX platform. Returns `null` if running in a mock mode.

**Not yet implemented.**

### `worker.onMessage(listener:Function) : Function`

Registers a `listener` for messages coming from `worker`.

### `worker.removeOnMessage(listener:Function)`

Unregisters a `listener`.

### `worker.postMessage(message:Object)`

Sends a `message` to the `worker`, so that listeners inside get it.
`message` should be JSON-serializable.

### `worker.terminate()`

Stops the `worker`.

**Not yet implemented.**

## API (inside, trusted)

### `SecureWorker.ready : Promise`

A Promise which resolves once worker has fully loaded and all APIs and services have initialized.

### `SecureWorker.getName() : String`

Returns the name under which the current script is stored in an enclave.

**Not yet implemented.**

### `SecureWorker.onMessage(listener:Function) : Function`

Registers a `listener` for messages coming from outside.

**Make sure you validate all messages before using them.**

### `SecureWorker.removeOnMessage(listener:Function)`

Unregisters a `listener`.

### `SecureWorker.postMessage(message:Object)`

Sends a `message` to outside, so that listeners there get it.
`message` should be JSON-serializable.

***Make sure you do not leak any sensitive data from the enclave.**

### `SecureWorker.close()`

Stops the `worker`.

**Not yet implemented.**

### `SecureWorker.importScripts(contentKeys:String...)`

Synchronously run scripts under given names.

### `SecureWorker.monotonicCounters.create() : {uuid:ArrayBuffer, value:Number}`

Creates a new monotonic counter. `uuid` represents ID of the counter, and `value`
is its current value. Monotonic counters are provided by the SGX platform
and persist even across enclave resets, so you can use them to prevent rollback
attacks.

### `SecureWorker.monotonicCounters.destroy(counterId:ArrayBuffer)`

Destroys a monotonic counter with ID `counterId`.

### `SecureWorker.monotonicCounters.read(counterId:ArrayBuffer) : Number`

Returns the value of a monotonic counter with ID `counterId`.

### `SecureWorker.monotonicCounters.increment(counterId:ArrayBuffer) : Number`

Increases a monotonic counter with ID `counterId` and returns the new value.

### `SecureWorker.getTrustedTime() : {currentTime:ArrayBuffer, timeSourceNonce:ArrayBuffer}`

`currentTime` is of the second resolution in the little-endian byte ordering.
It can be used as a trusted source of elapsed time. This is not clock time.
All this holds while `timeSourceNonce` stays the same. If `timeSourceNonce` changes,
then the trusted time has restarted as well.

### `SecureWorker.getReport([reportData:ArrayBuffer], [targetInfo:ArrayBuffer]) : ArrayBuffer`

Returns a report for the calling enclave. Report can optionally include 64 bytes of `reportData`.

If `targetInfo` is not provided (is `undefined`) it will be automatically populated
with target information needed for quoting.

### `crypto.subtle.*`

A cryptographic API. It matches
[Web Crypto API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Crypto_API).

### `crypto.random.getRandomValues(buffer:TypedArray) : TypedArray`

A source of trusted randomness. It matches
[Web Crypto API](https://developer.mozilla.org/en-US/docs/Web/API/RandomSource/getRandomValues).

### `Promise:Function`

[Promise](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise)
[implementation](https://github.com/taylorhakes/promise-polyfill).

### `nextTick:Function` and `setImmediate:Function`

Enclaves do not have IO, so those behave the same.

## Mock API

To use mock mode of operation, you have to provide a function which resolves
script names to their content. For example:

```javascript
import fs from 'fs';
import path from 'path';
import SecureWorker from 'secureworker';

SecureWorker._resolveContentKey = function _resolveContentKey(enclaveName, contentKey) {
  return fs.readFileSync(path.join(__dirname, contentKey), 'utf8');
};
```

If you want to force loading the package in the mock mode, set `FORCE_MOCK_SECUREWORKER`
environment variable.

## Examples

To see example of package's use, check the [Luckychain project](https://github.com/luckychain/lucky),
a proof of luck [IPFS](https://ipfs.io/) based blockchain:
* [its enclave code](https://github.com/luckychain/lucky/blob/master/enclave/luckychain.js)
* [its code using enclave](https://github.com/luckychain/lucky/blob/master/src/js/enclave.js)

Simpler examples can be found in [tests](https://github.com/luckychain/node-secureworker/tree/master/tests) as well.

