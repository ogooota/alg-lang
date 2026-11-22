#lang pt
// soma, maximo e busca binaria
ler(n, a[0..n])
escrever(soma(a[0..n]), max(a[0..n]), min(a[0..n]))
escrever("quadrados:", soma(i * i para i=0 ate n))
escrever("ate em:", soma(i para i=1 ate em n))

quadrado(x) = x * x

fn fib(k) {
    se k < 2 {
        ret k
    }
    ret fib(k - 1) + fib(k - 2)
}

escrever(quadrado(7), quadrado(1.5), fib(20))
escrever(-7 / 2, -7 % 2, 7 / 2, 7.0 / 2, 1 << 40)

v = {}
para x em a {
    se x % 2 == 0 {
        v.adicionar(x)
    }
}
escrever(v)
escrever(tam(v))

fn busca(w, alvo) {
    lo = 0
    hi = tam(w)
    enquanto lo < hi {
        m = (lo + hi) / 2
        se w[m] < alvo {
            lo = m + 1
        } senao {
            hi = m
        }
    }
    ret lo
}

ordenado = {1, 3, 5, 7, 9}
escrever(busca(ordenado, 7), busca(ordenado, 0), busca(ordenado, 10))
escrever(soma(ordenado[1..3]))
