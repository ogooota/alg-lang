#lang pt
#decl
    a[3];
    b[3];
    c[3];
    d[4]: int32;

para k=0 ate 3:
    a[k] = 0 - k * 40
    b[k] = 0 - k * 10000
    c[k] = k * 30000
escrever(a, b, c)
escrever(a[2] * c[2] * 1000)
d[0] = 2000000000
d[1] = d[0] + d[0] - 4000000000
escrever(d)
