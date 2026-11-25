#lang en
#decl {
    h: heap {(int, int)};
    big: heap(greater) {int};
    q: queue {str};
}
fn bfs(g: graph, s: int, n: int) -> {int} {
    dist = {}
    for i=0 to n dist.add(-1)
    fr = queue()
    fr.push(s)
    dist[s] = 0
    while (fr.size() > 0) {
        u = fr.pop()
        for v in g.neighbors(u)
            if (dist[v] < 0) {
                dist[v] = dist[u] + 1
                fr.push(v)
            }
    }
    ret dist
}
fn top_of(x: heap {(int, int)}) -> (int, int) {
    ret x.top()
}
h.push((3, 1))
h.push((1, 2))
big.push(4)
big.push(9)
q.push("a")
q.push("b")
write(top_of(h), big.top(), q.pop(), q.size())
g = graph(4)
g.edge(0, 1)
g.edge(1, 2)
write(bfs(g, 0, 4))
d = graph(3, directed)
d.edge(0, 1)
