#lang en
read(n, m)
edges = {}
for i=0 to m {
    read(u, v, w)
    edges.add((w, u - 1, v - 1))
}
sort(edges)
d = dsu(n)
total = 0
for (w, u, v) in edges
    if d.merge(u, v)
        total += w
write(total)
