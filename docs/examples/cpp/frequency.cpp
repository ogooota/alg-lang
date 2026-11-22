#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    cin >> n;
    map<long long, int> count;
    for (int i = 0; i < n; i++) {
        long long x;
        cin >> x;
        count[x]++;
    }
    for (auto [value, times] : count)
        cout << value << ' ' << times << '\n';
}
