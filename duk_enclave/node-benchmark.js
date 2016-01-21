var fs = require('fs');
var vm = require('vm');
vm.runInThisContext(fs.readFileSync('sunspider-scripts-only.js'));

for (var i = 0; i < 26; i++) {
    var start = process.hrtime();
    vm.runInNewContext(testContents[0]);
    var duration = process.hrtime(start);
    console.log((duration[0] * 1E9 + duration[1]) / 1000 + ' us');
}
