const { readInt } = require("./io.js");
const n = readInt();
const q = readInt();
const parent = new Int32Array(n);
const size = new Int32Array(n).fill(1);
for (let i = 0; i < n; i++) parent[i] = i;
function find(x) {
    while (parent[x] !== x) { parent[x] = parent[parent[x]]; x = parent[x]; }
    return x;
}
let groups = n, same = 0;
for (let k = 0; k < q; k++) {
    const t = readInt();
    let a = find(readInt()), b = find(readInt());
    if (t === 1) {
        if (a !== b) {
            if (size[a] < size[b]) [a, b] = [b, a];
            parent[b] = a;
            size[a] += size[b];
            groups--;
        }
    } else if (a === b) same++;
}
console.log(same, groups);
