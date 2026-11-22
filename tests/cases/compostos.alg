#lang pt
x = 10
x += 5
x -= 3
x *= 2
x /= 4
x %= 4
escrever(x)
y = 6
y &= 3
y |= 8
y ^= 1
y <<= 2
y >>= 1
escrever(y)
i = 0
inc i
inc i
dec i
escrever(i)
v = {1, 2, 3}
v[1] += 10
inc v[2]
escrever(v)
m = mapa {}
para k em {3, 1, 3}:
    m[k] += 1
para (k, c) em m:
    escrever(k, c)
r = 1.5
r *= 2
escrever(r)
escrever(5 - -3, 7 - -2)
