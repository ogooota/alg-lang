#lang en
read(n, q)
f = fenwick(n)
total = 0
for i=0 to q {
    read(t, a, b)
    if t == 1
        f.add(a, b)
    else
        total += f.sum(a, b)
}
write(total)
