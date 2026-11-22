#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    cin >> n;
    vector<pair<long long, long long>> tasks(n);
    for (auto &[end, start] : tasks)
        cin >> start >> end;
    sort(tasks.begin(), tasks.end());
    int count = 0;
    long long free_at = 0;
    for (auto [end, start] : tasks)
        if (start >= free_at) {
            count++;
            free_at = end;
        }
    cout << count << '\n';
}
