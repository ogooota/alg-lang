#lang pt
#decl dist[n];

ler(n, m)
g = grafo(n)
para i=0 ate m {
    ler(u, v)
    g.aresta(u - 1, v - 1)
}
para i=0 ate n {
    dist[i] = -1
}
dist[0] = 0
proximos = fila()
proximos.enfileirar(0)
enquanto proximos.tam() > 0 {
    u = proximos.desenfileirar()
    para v em g.vizinhos(u) {
        se dist[v] == -1 {
            dist[v] = dist[u] + 1
            proximos.enfileirar(v)
        }
    }
}
escrever(dist[n - 1])
