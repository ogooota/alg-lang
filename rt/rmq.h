/* Template de sparse table: ELEMENT, RMQ, FN, COMBINE (min ou max, operação idempotente).
 * level[j][i] = COMBINE de [i, i + 2^j); consulta em O(1) com dois blocos sobrepostos. */

typedef struct
{
    ELEMENT **level;
    int64_t count;
} RMQ;

/** Sparse table de n posições; level[0] é preenchido depois e FN(_build) faz o resto. */
static RMQ *FN(_new)(int64_t count)
{
    RMQ *table = arena_allocate(sizeof(RMQ));
    int levels = 1;

    while (((int64_t)1 << levels) <= count)
    {
        levels++;
    }

    table->level = arena_allocate((size_t)levels * sizeof(ELEMENT *));
    table->count = count;

    for (int j = 0; j < levels; j++)
    {
        table->level[j] = arena_allocate((size_t)(count + 1) * sizeof(ELEMENT));
    }

    return table;
}

/** Calcula os níveis 1, 2, ... a partir do nível 0. */
static RMQ *FN(_build)(RMQ *table)
{
    for (int j = 1; ((int64_t)1 << j) <= table->count; j++)
    {
        int64_t half = (int64_t)1 << (j - 1);

        for (int64_t i = 0; i + 2 * half <= table->count; i++)
        {
            table->level[j][i] = COMBINE(table->level[j - 1][i], table->level[j - 1][i + half]);
        }
    }

    return table;
}

/** Combinação das posições [low, high). */
static ELEMENT FN(_query)(RMQ *table, int64_t low, int64_t high, int line)
{
    if (ALG_CHECK && (low < 0 || high <= low || high > table->count))
    {
        runtime_error(line, "consulta(%lld, %lld) inválida (intervalo vazio ou fora de 0..%lld)",
                      (long long)low, (long long)high, (long long)table->count);
    }

    int j = 63 - __builtin_clzll((unsigned long long)(high - low));
    return COMBINE(table->level[j][low], table->level[j][high - ((int64_t)1 << j)]);
}
