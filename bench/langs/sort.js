const { readInt, readBig } = require("./io.js");
const n = readInt();
const v = new BigInt64Array(n);
for (let i = 0; i < n; i++) v[i] = readBig();
v.sort();
let total = 0;
for (let i = 0; i < n; i++) total = (total + Number(v[i] % 1000003n) * (i % 1000)) % 1000000007;
console.log(total);
