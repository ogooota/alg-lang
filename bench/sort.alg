#lang pt
ler(n, v[0..n])
ordenar(v)
total = 0
para i=0 ate n:
    total = (total + v[i] % 1000003 * (i % 1000)) % 1000000007
escrever(total)
