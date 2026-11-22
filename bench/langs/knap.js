const { readInt } = require("./io.js");
const n = readInt();
const cap = readInt();
const w = new Int32Array(n);
const v = new Float64Array(n);
for (let i = 0; i < n; i++) w[i] = readInt();
for (let i = 0; i < n; i++) v[i] = readInt();
const dp = new Float64Array(cap + 1);
for (let i = 0; i < n; i++) {
    const wi = w[i], vi = v[i];
    for (let c = cap; c >= wi; c--) {
        const take = dp[c - wi] + vi;
        if (take > dp[c]) dp[c] = take;
    }
}
console.log(dp[cap]);
