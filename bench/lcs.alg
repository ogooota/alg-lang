#lang en
#decl {
    dp[n + 1][m + 1]: int32;
    a: int32;
    b: int32;
}

read(n, m, a[0..n], b[0..m])
for i=1 thr n {
    for j=1 thr m {
        if a[i - 1] == b[j - 1] {
            dp[i][j] = dp[i - 1][j - 1] + 1
        } else {
            dp[i][j] = max(dp[i - 1][j], dp[i][j - 1])
        }
    }
}
write(dp[n][m])
