#include <bits/stdc++.h>
using namespace std;

struct Fenwick {
    int n;
    vector<long long> tree;
    Fenwick(int n) : n(n), tree(n + 1, 0) {}
    void add(int i, long long x) {
        for (i++; i <= n; i += i & -i) tree[i] += x;
    }
    long long prefix(int i) {
        long long s = 0;
        for (; i > 0; i -= i & -i) s += tree[i];
        return s;
    }
    long long sum(int l, int r) { return prefix(r) - prefix(l); }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, q;
    cin >> n >> q;
    Fenwick f(n);
    long long total = 0;
    while (q--) {
        int t, a, b;
        cin >> t >> a >> b;
        if (t == 1) f.add(a, b);
        else total += f.sum(a, b);
    }
    cout << total << '\n';
}
