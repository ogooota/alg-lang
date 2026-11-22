#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, q;
    cin >> n >> q;
    vector<long long> a(n), pre(n + 1, 0);
    for (auto &x : a) cin >> x;
    for (int i = 0; i < n; i++)
        pre[i + 1] = pre[i] + a[i];
    while (q--) {
        int l, r;
        cin >> l >> r;
        cout << pre[r] - pre[l - 1] << '\n';
    }
}
