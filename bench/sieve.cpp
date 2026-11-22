#include <bits/stdc++.h>
using namespace std;

int main()
{
    int n;
    cin >> n;
    vector<char> composto(n + 1, 0);
    int total = 0;
    for (long long i = 2; i <= n; i++)
        if (!composto[i])
        {
            total++;
            for (long long j = i * i; j <= n; j += i)
                composto[j] = 1;
        }
    cout << total << '\n';
}
