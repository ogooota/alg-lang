#include "io.h"

/* tabela hash de endereçamento aberto (C não tem uma na biblioteca padrão) */
int main(void)
{
    int64_t n = read_int();
    size_t capacity = 1 << 21;
    int64_t *keys = malloc(capacity * sizeof *keys);
    int64_t *counts = calloc(capacity, sizeof *counts);
    int64_t distinct = 0;
    int64_t best = 0;

    for (int64_t i = 0; i < n; i++)
    {
        int64_t key = read_int();
        size_t slot = ((uint64_t)key * 0x9E3779B97F4A7C15ULL) >> 43;

        while (counts[slot] && keys[slot] != key)
        {
            slot = (slot + 1) & (capacity - 1);
        }

        if (!counts[slot])
        {
            keys[slot] = key;
            distinct++;
        }

        counts[slot]++;
        best = counts[slot] > best ? counts[slot] : best;
    }

    write_int(distinct, ' ');
    write_int(best, '\n');
    flush_output();
}
