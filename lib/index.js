if (process.env.FORCE_MOCK_SECUREWORKER) {
  console.warn("Mock SGX secure worker forced. Do not use in production.");
  module.exports = require('./mock');
}
else {
  try {
    module.exports = require('./real');
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
    module.exports = require('./mock');
  }
}
