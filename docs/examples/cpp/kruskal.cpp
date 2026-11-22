#include <bits/stdc++.h>
using namespace std;

struct DSU {
    vector<int> parent, size;
    DSU(int n) : parent(n), size(n, 1) { iota(parent.begin(), parent.end(), 0); }
    int find(int x) {
        while (parent[x] != x) x = parent[x] = parent[parent[x]];
        return x;
    }
    bool merge(int a, int b) {
        a = find(a), b = find(b);
        if (a == b) return false;
        if (size[a] < size[b]) swap(a, b);
        parent[b] = a;
        size[a] += size[b];
        return true;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, m;
    cin >> n >> m;
    vector<tuple<long long, int, int>> edges(m);
    for (auto &[w, u, v] : edges) {
        cin >> u >> v >> w;
        u--, v--;
    }
    sort(edges.begin(), edges.end());
    DSU d(n);
    long long total = 0;
    for (auto [w, u, v] : edges)
        if (d.merge(u, v)) total += w;
    cout << total << '\n';
}
