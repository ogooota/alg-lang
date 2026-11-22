#lang pt
#decl pre[n + 1];

ler(n, q, a[0..n])
para i=0 ate n {
    pre[i + 1] = pre[i] + a[i]
}
para k=0 ate q {
    ler(l, r)
    escrever(pre[r] - pre[l - 1])
}
