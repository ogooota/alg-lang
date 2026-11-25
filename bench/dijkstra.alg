#lang en
#decl dist[n];

read(n, m)
g = graph(n, directed)
for i=0 to m {
    read(u, v, w)
    g.edge(u, v, w)
}
for i=0 to n {
    dist[i] = INF
}
dist[0] = 0
h = heap()
h.push((0, 0))
while h.size() > 0 {
    (d, u) = h.pop()
    if d > dist[u] {
        continue
    }
    for (v, w) in g.neighbors(u) {
        if d + w < dist[v] {
            dist[v] = d + w
            h.push((dist[v], v))
        }
    }
}
total = 0
for i=0 to n {
    if dist[i] < INF {
        total = total + dist[i]
    }
}
write(total)
