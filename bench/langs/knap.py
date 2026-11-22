import sys
data = sys.stdin.buffer.read().split()
n, cap = int(data[0]), int(data[1])
w = list(map(int, data[2:2 + n]))
v = list(map(int, data[2 + n:2 + 2 * n]))
dp = [0] * (cap + 1)
for i in range(n):
    wi, vi = w[i], v[i]
    for c in range(cap, wi - 1, -1):
        take = dp[c - wi] + vi
        if take > dp[c]:
            dp[c] = take
print(dp[cap])
