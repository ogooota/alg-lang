#lang pt
#decl dp[w + 1];

ler(n, w)
para i=0 ate n {
    ler(peso, valor)
    para c=0 ate em w - peso {
        cap = w - c
        dp[cap] = max(dp[cap], dp[cap - peso] + valor)
    }
}
escrever(dp[w])
