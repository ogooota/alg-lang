#lang en
read(n, q, v[0..n])
s = segtree(v, min)
total = 0
for i=0 to q {
    read(t, a, b)
    if t == 1
        s.update(a, b)
    else
        total += s.query(a, b)
}
write(total)
