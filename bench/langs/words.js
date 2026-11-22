const { readInt, readWord } = require("./io.js");
const n = readInt();
const count = new Map();
for (let i = 0; i < n; i++) {
    const w = readWord();
    count.set(w, (count.get(w) || 0) + 1);
}
const items = [...count.keys()].sort();
const out = new Array(items.length);
for (let i = 0; i < items.length; i++) out[i] = items[i] + " " + count.get(items[i]);
process.stdout.write(out.join("\n") + "\n");
