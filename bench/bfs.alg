#lang pt
#decl dist[n]: int32;

ler(n, m)
g = grafo(n)
para i=0 ate m {
    ler(u, v)
    g.aresta(u, v)
}
para i=0 ate n {
    dist[i] = -1
}
dist[0] = 0
q = fila()
q.enfileirar(0)
enquanto tam(q) > 0 {
    u = q.desenfileirar()
    para v em g.vizinhos(u) {
        se dist[v] == -1 {
            dist[v] = dist[u] + 1
            q.enfileirar(v)
        }
    }
}
escrever(soma(dist), max(dist))
