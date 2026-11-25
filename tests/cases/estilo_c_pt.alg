#lang pt
s = 0
para (i = 0; i < 4; i++) s += i
para (j = 3; j >= 0; j--) {
    se (j == 1) continue
    s += 100
}
para (i = 0 ate 2) s++
para (x em {7, 8}) s += x
enquanto (s % 10 != 0) s++
--s
se (s > 0) escrever(s) senao escrever("não")
