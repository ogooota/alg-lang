#lang en
#decl dist[n];

read(n, m)
g = graph(n)
for i=0 to m {
    read(u, v)
    g.edge(u - 1, v - 1)
}
for i=0 to n {
    dist[i] = -1
}
dist[0] = 0
pending = queue()
pending.push(0)
while pending.size() > 0 {
    u = pending.pop()
    for v in g.neighbors(u) {
        if dist[v] == -1 {
            dist[v] = dist[u] + 1
            pending.push(v)
        }
    }
}
write(dist[n - 1])
