#include <bits/stdc++.h>
using namespace std;

long long fib(int k)
{
    if (k < 2)
        return k;
    return fib(k - 1) + fib(k - 2);
}

int main()
{
    int n;
    cin >> n;
    cout << fib(n) << '\n';
}
