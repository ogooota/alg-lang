#lang pt
#decl {
    dp[cap + 1];
    w: int32;
}

ler(n, cap, w[0..n], v[0..n])
para i=0 ate n {
    para j=0 ate em cap - w[i] {
        c = cap - j
        dp[c] = max(dp[c], dp[c - w[i]] + v[i])
    }
}
escrever(dp[cap])
