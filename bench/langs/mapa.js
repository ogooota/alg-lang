const { readInt } = require("./io.js");
const n = readInt();
const count = new Map();
let best = 0;
for (let i = 0; i < n; i++) {
    const x = readInt();
    const c = (count.get(x) || 0) + 1;
    count.set(x, c);
    if (c > best) best = c;
}
console.log(count.size, best);
