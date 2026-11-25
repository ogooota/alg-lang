#include <bits/stdc++.h>
using namespace std;

int main() {
    int k;
    cin >> k;
    string line;
    getline(cin >> ws, line);
    string answer;
    for (char c : line) {
        if (c >= 'a' && c <= 'z')
            c = 'a' + ((c - 'a' + k) % 26 + 26) % 26;
        answer += c;
    }
    cout << answer << '\n';
}
