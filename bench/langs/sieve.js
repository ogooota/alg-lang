const { readInt } = require("./io.js");
const n = readInt();
const composite = new Uint8Array(n + 1);
let total = 0;
for (let i = 2; i <= n; i++) {
    if (!composite[i]) {
        total++;
        for (let j = i * i; j <= n; j += i) composite[j] = 1;
    }
}
console.log(total);
