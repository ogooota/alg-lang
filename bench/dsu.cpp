#include <bits/stdc++.h>
using namespace std;

struct DSU {
    vector<int> parent, size;
    int groups;
    DSU(int n) : parent(n), size(n, 1), groups(n) { iota(parent.begin(), parent.end(), 0); }
    int find(int x) {
        while (parent[x] != x) x = parent[x] = parent[parent[x]];
        return x;
    }
    void merge(int a, int b) {
        a = find(a), b = find(b);
        if (a == b) return;
        if (size[a] < size[b]) swap(a, b);
        parent[b] = a;
        size[a] += size[b];
        groups--;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, q;
    cin >> n >> q;
    DSU d(n);
    long long same = 0;
    while (q--) {
        int t, u, v;
        cin >> t >> u >> v;
        if (t == 1) d.merge(u, v);
        else if (d.find(u) == d.find(v)) same++;
    }
    cout << same << ' ' << d.groups << '\n';
}
