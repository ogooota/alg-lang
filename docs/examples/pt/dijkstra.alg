#lang pt
#decl dist[n];

ler(n, m)
g = grafo(n, dirigido)
para i=0 ate m {
    ler(u, v, w)
    g.aresta(u - 1, v - 1, w)
}
para i=0 ate n {
    dist[i] = INF
}
dist[0] = 0
h = heap()
h.inserir((0, 0))
enquanto tam(h) > 0 {
    (d, u) = h.remover()
    se d > dist[u] {
        continue
    }
    para (v, w) em g.vizinhos(u) {
        se d + w < dist[v] {
            dist[v] = d + w
            h.inserir((dist[v], v))
        }
    }
}
escrever(dist)
