#include <bits/stdc++.h>
using namespace std;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    cin >> n;
    vector<string> w(n);

    for (auto &s : w)
    {
        cin >> s;
    }

    sort(w.begin(), w.end());

    for (auto &s : w)
    {
        cout << s << '\n';
    }
}
