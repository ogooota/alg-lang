#lang pt
#decl {
    dp[n + 1][m + 1]: int32;
    a: int32;
    b: int32;
}

ler(n, m, a[0..n], b[0..m])
para i=1 ate em n {
    para j=1 ate em m {
        se a[i - 1] == b[j - 1] {
            dp[i][j] = dp[i - 1][j - 1] + 1
        } senao {
            dp[i][j] = max(dp[i - 1][j], dp[i][j - 1])
        }
    }
}
escrever(dp[n][m])
