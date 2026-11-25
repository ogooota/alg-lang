#lang pt
// tuplas
p = (3, 2.5)
(a, b) = p
escrever(p, a, b, (1, 2) < (1, 3), (2, 0) < (1, 9), (1, 2) == (1, 2))

// ordenar: inteiros, decrescente, fatia, tuplas, reais
v = {5, -3, 9, 0, -3, 7}
ordenar(v)
escrever(v)
ordenar(v, maior)
escrever(v)
w = {9, 8, 7, 6, 5}
ordenar(w[1..4])
escrever(w)
pares = {(2, 1), (1, 5), (2, 0), (1, 2)}
ordenar(pares)
escrever(pares)
r = {2.5, -1.0, 3.25}
ordenar(r)
escrever(r)

// heap
h = heap()
para x em {5, 1, 4, 2} {
    h.inserir(x)
}
escrever(h.topo(), h.tam())
enquanto h.tam() > 0 {
    escrever(h.remover())
}
hm = heap(maior)
hm.inserir((1, 10))
hm.inserir((3, 30))
hm.inserir((2, 20))
(d, u) = hm.remover()
escrever(d, u)

// fila
f = fila()
para i=0 ate 5 {
    f.enfileirar(i * i)
}
escrever(f.desenfileirar(), f.frente(), f.tam())

// mapa: contagem, em, remover, percorrer em ordem
cont = mapa {}
para x em {3, 1, 3, 2, 3, 1} {
    cont[x] = cont[x] + 1
}
escrever(cont[3], cont[7], 2 em cont, 7 em cont, cont.tam())
cont.remover(2)
para (k, c) em cont {
    escrever(k, c)
}
pos = mapa {}
pos[(1, 2)] = 5
escrever(pos[(1, 2)], (2, 1) em pos)

// grafo + fila: BFS
g = grafo(6)
g.aresta(0, 1)
g.aresta(0, 2)
g.aresta(1, 3)
g.aresta(2, 4)
dist = {-1, -1, -1, -1, -1, -1}
dist[0] = 0
q = fila()
q.enfileirar(0)
enquanto q.tam() > 0 {
    x = q.desenfileirar()
    para y em g.vizinhos(x) {
        se dist[y] == -1 {
            dist[y] = dist[x] + 1
            q.enfileirar(y)
        }
    }
}
escrever(dist)

// Dijkstra com INF e int32
gd = grafo(4, dirigido)
gd.aresta(0, 1, 5)
gd.aresta(0, 2, 1)
gd.aresta(2, 1, 2)
gd.aresta(1, 3, 1)
fn dijkstra(gr, origem, n) {
    dd = {INF, INF, INF, INF}
    dd[origem] = 0
    hp = heap()
    hp.inserir((0, origem))
    enquanto hp.tam() > 0 {
        (du, uu) = hp.remover()
        se du > dd[uu] {
            continue
        }
        para (vv, ww) em gr.vizinhos(uu) {
            se du + ww < dd[vv] {
                dd[vv] = du + ww
                hp.inserir((dd[vv], vv))
            }
        }
    }
    ret dd
}
escrever(dijkstra(gd, 0, 4))
escrever(dijkstra(gd, 3, 4))
