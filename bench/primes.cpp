#include <bits/stdc++.h>
using namespace std;

int main() {
    int n;
    cin >> n;
    vector<char> composite(n + 1, 0);
    vector<int> primes;
    for (long long i = 2; i <= n; i++) {
        if (composite[i]) continue;
        primes.push_back(i);
        for (long long j = i * i; j <= n; j += i) composite[j] = 1;
    }
    long long s = 0;
    for (int p : primes) s += p;
    cout << primes.size() << ' ' << s << '\n';
}
