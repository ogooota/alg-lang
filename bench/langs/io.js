// leitura dos bytes do stdin com parser manual (bem mais rápido que split em JS)
const data = require("fs").readFileSync(0);
let position = 0;

function readInt() {
    while (data[position] < 45) position++;
    let negative = false;
    if (data[position] === 45) { negative = true; position++; }
    let value = 0;
    while (data[position] >= 48) value = value * 10 + data[position++] - 48;
    return negative ? -value : value;
}

function readBig() {
    while (data[position] < 48) position++;
    const start = position;
    while (data[position] >= 48) position++;
    return BigInt(data.toString("latin1", start, position));
}

function readWord() {
    while (data[position] <= 32) position++;
    const start = position;
    while (position < data.length && data[position] > 32) position++;
    return data.toString("latin1", start, position);
}

module.exports = { readInt, readBig, readWord };
