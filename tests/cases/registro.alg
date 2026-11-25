#lang en
type Edge { w: int; u: int; v: int }
type Person {
    name: str
    score: real
}
#decl edges: {Edge}
read(n, edges[0..n])
sort(edges)
for e in edges write(e.u, e.v, e.w)
e = Edge(9, 1, 2)
e.w += 1
write(e, e.w)
[w, u, v] = e
write(w + u + v)
p = Person("ana", 7)
p.score = p.score / 2
write(p.name, p.score)
fn heavier(a: Edge, b: Edge) -> Edge {
    if (a.w > b.w) ret a
    ret b
}
write(heavier(edges[0], e).w)
best = map {}
best[Edge(1, 2, 3)] = 5
write(best[Edge(1, 2, 3)])
h = heap()
h.push(Edge(4, 0, 1))
h.push(Edge(2, 3, 1))
write(h.top().u)
