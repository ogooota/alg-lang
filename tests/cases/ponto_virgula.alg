#lang pt
// o ';' no fim é opcional: com e sem, no mesmo arquivo
x = 3;
y = 4
inc x;
fn soma2(a, b):
    ret a + b;
se x > 2: escrever("maior");
para i=0 ate 2:
    escrever(i, soma2(x, y));
    continue;
escrever(x, y)
