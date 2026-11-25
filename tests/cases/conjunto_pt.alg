#lang pt
a = conj {1, 2}
b = conj {2, 3}
para x em a uniao b escrever(x)
para x em a inter b escrever("i", x)
escrever((a menos b).tam())
