#lang pt
ler(n, grid[0..n][0..2])
para linha em grid {
    escrever(soma(linha))
}
v = {5, 1, 9}
ler(v)
escrever(v, max(x para x em v), min(x * 2 para x em v))
escrever(int(2.7), int(-2.5), real(3) / 2)
se 6 & 1 == 0 { escrever("par") }
x ← 3
se x ≤ 3 e x ≠ 4 { escrever("unicode ok") }
escrever(copia(v[1..3]), verdadeiro, falso)
ret
escrever("nao chega aqui")
