#lang pt
// conta_primos(n) contra crivo(n), e valores conhecidos de π(n)
erros = 0
para n=0 ate em 2000 {
    se (conta_primos(n) != crivo(n).tam()) erros++
}
escrever(erros, conta_primos(10000000000), conta_primos(-5), conta_primos(2))
