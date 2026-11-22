#include "io.h"

static int compare(const void *left, const void *right)
{
    int64_t a = *(const int64_t *)left;
    int64_t b = *(const int64_t *)right;
    return (a > b) - (a < b);
}

int main(void)
{
    int64_t n = read_int();
    int64_t *v = malloc(n * sizeof *v);

    for (int64_t i = 0; i < n; i++)
    {
        v[i] = read_int();
    }

    qsort(v, n, sizeof *v, compare);
    int64_t total = 0;

    for (int64_t i = 0; i < n; i++)
    {
        total = (total + v[i] % 1000003 * (i % 1000)) % 1000000007;
    }

    write_int(total, '\n');
    flush_output();
}
