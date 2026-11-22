#include <bits/stdc++.h>
using namespace std;

bool fits(const vector<long long> &weights, long long cap, int k) {
    int trips = 1;
    long long load = 0;
    for (long long w : weights) {
        if (w > cap) return false;
        if (load + w > cap) {
            trips++;
            load = 0;
        }
        load += w;
    }
    return trips <= k;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n, k;
    cin >> n >> k;
    vector<long long> weights(n);
    for (auto &w : weights) cin >> w;
    long long lo = 1, hi = accumulate(weights.begin(), weights.end(), 0LL);
    while (lo < hi) {
        long long mid = lo + (hi - lo) / 2;
        if (fits(weights, mid, k)) hi = mid;
        else lo = mid + 1;
    }
    cout << lo << '\n';
}
