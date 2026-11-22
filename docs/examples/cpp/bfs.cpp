#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, m;
    cin >> n >> m;
    vector<vector<int>> g(n);
    for (int i = 0; i < m; i++) {
        int u, v;
        cin >> u >> v;
        g[u - 1].push_back(v - 1);
        g[v - 1].push_back(u - 1);
    }
    vector<int> dist(n, -1);
    dist[0] = 0;
    queue<int> pending;
    pending.push(0);
    while (!pending.empty()) {
        int u = pending.front();
        pending.pop();
        for (int v : g[u])
            if (dist[v] == -1) {
                dist[v] = dist[u] + 1;
                pending.push(v);
            }
    }
    cout << dist[n - 1] << '\n';
}
