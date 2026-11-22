#lang pt
cont = mapa {}
ler(n)
para i=0 ate n:
    ler(x)
    cont[x] = cont[x] + 1
melhor = 0
para (k, c) em cont:
    melhor = max(melhor, c)
escrever(tam(cont), melhor)
