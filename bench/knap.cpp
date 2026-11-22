#include <bits/stdc++.h>
using namespace std;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, cap;
    cin >> n >> cap;
    vector<int> w(n);
    vector<long long> v(n), dp(cap + 1, 0);
    for (auto &x : w)
        cin >> x;
    for (auto &x : v)
        cin >> x;
    for (int i = 0; i < n; i++)
        for (int c = cap; c >= w[i]; c--)
            dp[c] = max(dp[c], dp[c - w[i]] + v[i]);
    cout << dp[cap] << '\n';
}
