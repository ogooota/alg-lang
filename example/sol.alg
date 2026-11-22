#lang pt
#decl 
    ev[n];

ler(n)
pols = 0 
unt = 0
para i = 0 ate n:
    ler(ev[i])
    se ev[i] == -1:
        se pols > 0:
            dec pols 
            continue
        inc unt
        continue
    pols += ev[i]

escrever(unt)


