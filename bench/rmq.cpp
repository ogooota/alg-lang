#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, q;
    cin >> n >> q;
    int levels = __lg(n) + 1;
    vector<vector<long long>> table(levels, vector<long long>(n));
    for (auto &x : table[0]) cin >> x;
    for (int j = 1; j < levels; j++)
        for (int i = 0; i + (1 << j) <= n; i++)
            table[j][i] = min(table[j - 1][i], table[j - 1][i + (1 << (j - 1))]);
    long long total = 0;
    while (q--) {
        int l, r;
        cin >> l >> r;
        int j = __lg(r - l);
        total += min(table[j][l], table[j][r - (1 << j)]);
    }
    cout << total << '\n';
}
