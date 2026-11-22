#lang pt
#decl dist[5]: int32;

para i=0 ate 5 {
    dist[i] = INF
}
dist[0] = 0
dist[2] = 7
escrever(dist[1] == INF, dist[2] == INF, dist[1] + 5 > INF, min(dist[1], dist[2] + 1))
escrever(dist)
ordenar(dist)
escrever(dist[0], dist[4] == INF)
grande = INF + INF
escrever(grande > INF, -INF < 0)
