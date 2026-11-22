#include <bits/stdc++.h>
using namespace std;

int main()
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    cin >> n;
    unordered_map<string, int> count;
    string word;

    for (int i = 0; i < n; i++)
    {
        cin >> word;
        count[word]++;
    }

    vector<pair<string, int>> items(count.begin(), count.end());
    sort(items.begin(), items.end());

    for (auto &[w, k] : items)
    {
        cout << w << ' ' << k << '\n';
    }
}
