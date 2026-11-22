#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, w;
    cin >> n >> w;
    vector<long long> best(w + 1, 0);
    for (int i = 0; i < n; i++) {
        int weight;
        long long value;
        cin >> weight >> value;
        for (int cap = w; cap >= weight; cap--)
            best[cap] = max(best[cap], best[cap - weight] + value);
    }
    cout << best[w] << '\n';
}
