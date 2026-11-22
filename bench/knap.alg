#lang en
#decl {
    dp[cap + 1];
    w: int32;
}

read(n, cap, w[0..n], v[0..n])
for i=0 to n {
    for j=0 thr cap - w[i] {
        c = cap - j
        dp[c] = max(dp[c], dp[c - w[i]] + v[i])
    }
}
write(dp[cap])
