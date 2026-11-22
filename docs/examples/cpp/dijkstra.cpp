#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, m;
    cin >> n >> m;
    vector<vector<pair<int, long long>>> g(n);
    for (int i = 0; i < m; i++) {
        int u, v;
        long long w;
        cin >> u >> v >> w;
        g[u - 1].push_back({v - 1, w});
    }
    const long long INF = 1e18;
    vector<long long> dist(n, INF);
    dist[0] = 0;
    priority_queue<pair<long long, int>, vector<pair<long long, int>>, greater<>> h;
    h.push({0, 0});
    while (!h.empty()) {
        auto [d, u] = h.top();
        h.pop();
        if (d > dist[u]) continue;
        for (auto [v, w] : g[u])
            if (d + w < dist[v]) {
                dist[v] = d + w;
                h.push({dist[v], v});
            }
    }
    for (int i = 0; i < n; i++)
        cout << dist[i] << " \n"[i == n - 1];
}
