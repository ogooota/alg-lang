#lang pt
ler(n, m)
arestas = {}
para i=0 ate m {
    ler(u, v, w)
    arestas.adicionar((w, u - 1, v - 1))
}
ordenar(arestas)
d = conjuntos(n)
total = 0
para [w, u, v] em arestas
    se d.unir(u, v)
        total += w
escrever(total)
