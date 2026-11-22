#include <bits/stdc++.h>
using namespace std;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    cin >> n;
    unordered_map<long long, int> cont;
    for (int i = 0; i < n; i++)
    {
        long long x;
        cin >> x;
        cont[x]++;
    }
    int melhor = 0;
    for (auto &[k, c] : cont)
        melhor = max(melhor, c);
    cout << cont.size() << ' ' << melhor << '\n';
}
