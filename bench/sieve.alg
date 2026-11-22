#lang pt
#decl composto[n + 1];

ler(n)
total = 0
para i=2 ate em n {
    se nao composto[i] {
        total = total + 1
        j = i * i
        enquanto j <= n {
            composto[j] = 1
            j = j + i
        }
    }
}
escrever(total)
