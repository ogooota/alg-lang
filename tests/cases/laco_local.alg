#lang pt
// o 'para' dentro de uma função não pode mexer no 'i' do programa principal
#decl visitado[5];

fn marca(k):
    para i=0 ate em k:
        visitado[i] = 1

fn conta(v):
    total = 0
    para x em v:
        total = total + x
    ret total

x = 100
para i=0 ate 5:
    marca(i)
    escrever(i, conta(visitado))
escrever("x continua", x)
