#lang pt
tipo Aresta { peso: int; a: int; b: int }
x = Aresta(3, 1, 2)
x.peso *= 2
escrever(x.peso, x)
