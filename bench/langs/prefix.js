const { readInt } = require("./io.js");
const n = readInt();
const prefix = new Float64Array(n + 1);
for (let i = 0; i < n; i++) prefix[i + 1] = prefix[i] + readInt();
const q = readInt();
const out = new Array(q);
for (let k = 0; k < q; k++) {
    const l = readInt();
    const r = readInt();
    out[k] = prefix[r] - prefix[l];
}
process.stdout.write(out.join("\n") + "\n");
