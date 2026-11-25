#lang en
#decl dist[n]: int32;

read(n, m)
g = graph(n)
for i=0 to m {
    read(u, v)
    g.edge(u, v)
}
for i=0 to n {
    dist[i] = -1
}
dist[0] = 0
q = queue()
q.push(0)
while q.size() > 0 {
    u = q.pop()
    for v in g.neighbors(u) {
        if dist[v] == -1 {
            dist[v] = dist[u] + 1
            q.push(v)
        }
    }
}
write(sum(dist), max(dist))
