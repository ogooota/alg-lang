const { readInt } = require("./io.js");
const n = readInt();
const m = readInt();
const from = new Int32Array(m), to = new Int32Array(m), weight = new Float64Array(m);
const start = new Int32Array(n + 1);
for (let i = 0; i < m; i++) {
    from[i] = readInt(); to[i] = readInt(); weight[i] = readInt();
    start[from[i] + 1]++;
}
for (let i = 0; i < n; i++) start[i + 1] += start[i];
const fill = start.slice(0, n);
const adjacent = new Int32Array(m), adjacentWeight = new Float64Array(m);
for (let i = 0; i < m; i++) {
    const slot = fill[from[i]]++;
    adjacent[slot] = to[i];
    adjacentWeight[slot] = weight[i];
}
// heap binário com dois vetores paralelos (distância, vértice)
const heapDistance = new Float64Array(m + 1), heapVertex = new Int32Array(m + 1);
let heapSize = 0;
function push(d, v) {
    let i = heapSize++;
    while (i > 0) {
        const p = (i - 1) >> 1;
        if (heapDistance[p] <= d) break;
        heapDistance[i] = heapDistance[p]; heapVertex[i] = heapVertex[p]; i = p;
    }
    heapDistance[i] = d; heapVertex[i] = v;
}
const dist = new Float64Array(n).fill(Infinity);
dist[0] = 0;
push(0, 0);
while (heapSize) {
    const d = heapDistance[0], u = heapVertex[0];
    const lastD = heapDistance[--heapSize], lastV = heapVertex[heapSize];
    let i = 0;
    while (2 * i + 1 < heapSize) {
        let c = 2 * i + 1;
        if (c + 1 < heapSize && heapDistance[c + 1] < heapDistance[c]) c++;
        if (heapDistance[c] >= lastD) break;
        heapDistance[i] = heapDistance[c]; heapVertex[i] = heapVertex[c]; i = c;
    }
    heapDistance[i] = lastD; heapVertex[i] = lastV;
    if (d > dist[u]) continue;
    for (let e = start[u]; e < start[u + 1]; e++) {
        const nd = d + adjacentWeight[e], v = adjacent[e];
        if (nd < dist[v]) { dist[v] = nd; push(nd, v); }
    }
}
let total = 0;
for (let i = 0; i < n; i++) if (dist[i] !== Infinity) total += dist[i];
console.log(total);
