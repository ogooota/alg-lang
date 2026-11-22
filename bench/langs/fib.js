const { readInt } = require("./io.js");
function fib(k) { return k < 2 ? k : fib(k - 1) + fib(k - 2); }
console.log(fib(readInt()));
