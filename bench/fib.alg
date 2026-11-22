#lang pt
fn fib(k) {
    se k < 2 {
        ret k
    }
    ret fib(k - 1) + fib(k - 2)
}

ler(n)
escrever(fib(n))
