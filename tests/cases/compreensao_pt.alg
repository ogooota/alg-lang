#lang pt
v = {3, -1, 4}
escrever({x * 2 para x em v se x > 0})
escrever(todo(x != 0 para x em v), existe(x > 3 para x em v), algum(x > 9 para x em v))
escrever(conj {x % 2 para x em v}.tam())
