#include <bits/stdc++.h>
using namespace std;

struct Segtree {
    int n;
    vector<long long> tree;
    Segtree(const vector<long long> &v) : n(v.size()), tree(2 * v.size()) {
        for (int i = 0; i < n; i++) tree[n + i] = v[i];
        for (int i = n - 1; i > 0; i--) tree[i] = min(tree[2 * i], tree[2 * i + 1]);
    }
    void set(int i, long long x) {
        for (tree[i += n] = x; i > 1; i /= 2) tree[i / 2] = min(tree[i], tree[i ^ 1]);
    }
    long long query(int l, int r) {
        long long res = LLONG_MAX;
        for (l += n, r += n; l < r; l /= 2, r /= 2) {
            if (l & 1) res = min(res, tree[l++]);
            if (r & 1) res = min(res, tree[--r]);
        }
        return res;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, q;
    cin >> n >> q;
    vector<long long> v(n);
    for (auto &x : v) cin >> x;
    Segtree s(v);
    long long total = 0;
    while (q--) {
        int t, a;
        long long b;
        cin >> t >> a >> b;
        if (t == 1) s.set(a, b);
        else total += s.query(a, b);
    }
    cout << total << '\n';
}
