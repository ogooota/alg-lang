#include <bits/stdc++.h>
using namespace std;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    cin >> n;
    vector<long long> v(n);
    for (auto &x : v)
        cin >> x;
    sort(v.begin(), v.end());
    long long total = 0;
    for (int i = 0; i < n; i++)
        total = (total + v[i] % 1000003 * (i % 1000)) % 1000000007;
    cout << total << '\n';
}
