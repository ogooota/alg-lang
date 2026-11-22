#lang en
read(n, q, v[0..n])
r = rmq(v, min)
total = 0
for i=0 to q {
    read(a, b)
    total += r.query(a, b)
}
write(total)
