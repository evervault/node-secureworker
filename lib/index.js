try {
  module.exports = require('real');
}
catch (error) {
  if (error.stack) {
    console.warn("Could not load SGX secure worker binary:");
    console.warn(error.stack);
  }
  else {
    console.warn("Could not load SGX secure worker binary: " + error);
  }
  console.warn("Mock implementation will be used instead. Do not use in production.");
  module.exports = require('mock');
}
