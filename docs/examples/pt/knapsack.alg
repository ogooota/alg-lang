#lang pt
#decl dp[w + 1];

ler(n, w)
para i=0 ate n {
    ler(peso, valor)
    para (cap = w; cap >= peso; cap--)
        dp[cap] = max(dp[cap], dp[cap - peso] + valor)
}
escrever(dp[w])
