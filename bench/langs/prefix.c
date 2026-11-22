#include "io.h"

int main(void)
{
    int64_t n = read_int();
    int64_t *prefix = calloc(n + 1, sizeof *prefix);

    for (int64_t i = 0; i < n; i++)
    {
        prefix[i + 1] = prefix[i] + read_int();
    }

    int64_t q = read_int();

    for (int64_t k = 0; k < q; k++)
    {
        int64_t low = read_int();
        int64_t high = read_int();
        write_int(prefix[high] - prefix[low], '\n');
    }

    flush_output();
}
