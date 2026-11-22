#lang en
read(n, q)
d = dsu(n)
same = 0
for i=0 to q {
    read(t, u, v)
    if t == 1
        d.merge(u, v)
    else if d.same(u, v)
        inc same
}
write(same, d.groups())
