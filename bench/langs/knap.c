#include "io.h"

int main(void)
{
    int64_t n = read_int();
    int64_t cap = read_int();
    int64_t *w = malloc(n * sizeof *w);
    int64_t *v = malloc(n * sizeof *v);
    int64_t *dp = calloc(cap + 1, sizeof *dp);

    for (int64_t i = 0; i < n; i++)
    {
        w[i] = read_int();
    }

    for (int64_t i = 0; i < n; i++)
    {
        v[i] = read_int();
    }

    for (int64_t i = 0; i < n; i++)
    {
        for (int64_t c = cap; c >= w[i]; c--)
        {
            int64_t take = dp[c - w[i]] + v[i];
            dp[c] = dp[c] > take ? dp[c] : take;
        }
    }

    write_int(dp[cap], '\n');
    flush_output();
}
