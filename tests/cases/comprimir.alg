#lang en
v = {1000000000, -5, 7, 7, 1000000000, 3}
write(compress(v))
r = {2.5, -1.0, 2.5}
write(compress(r))
s = {"bob", "ana", "bob", "carl"}
write(compress(s))
t = {(2, 1), (1, 9), (2, 1)}
write(compress(t))
e = {}
write(compress(e).size())
c = compress(v)
write(max(c) + 1)
