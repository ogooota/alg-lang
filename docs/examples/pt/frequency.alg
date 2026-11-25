#lang pt
ler(n)
cont = mapa {}
para i=0 ate n {
    ler(x)
    cont[x] += 1
}
para [valor, vezes] em cont {
    escrever(valor, vezes)
}
