#lang pt
#decl
    v[n];
    w[m];
    m;

fn le_m():
    ler(m)

// n só é lido dentro do laço: a criação de v não pode subir para antes dele
para t=0 ate 1:
    ler(n)
    para i=0 ate n:
        v[i] = i * i

// m muda dentro de uma função chamada no laço
k = 0
enquanto k < 1:
    le_m()
    w[m - 1] = 7
    k = k + 1
escrever(v, w)
