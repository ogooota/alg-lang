/* Template de Fenwick (árvore binária indexada): ELEMENT, FENWICK, FN.
 * tree[i] (1-based) guarda a soma de (i - lowbit(i), i]; somar e soma de prefixo em O(log n). */

typedef struct
{
    ELEMENT *tree;
    int64_t count;
} FENWICK;

/** Fenwick de n posições, todas zero. */
static FENWICK *FN(_new)(int64_t count, int line)
{
    if (count < 0)
    {
        runtime_error(line, "fenwick(%lld): tamanho negativo", (long long)count);
    }

    FENWICK *fenwick = arena_allocate(sizeof(FENWICK));
    fenwick->tree = arena_allocate((size_t)(count + 1) * sizeof(ELEMENT));
    fenwick->count = count;
    return fenwick;
}

/** Soma amount na posição index (0-based). */
static void FN(_add)(FENWICK *fenwick, int64_t index, ELEMENT amount, int line)
{
    /* cópias locais: escrever em tree não obriga o compilador a reler count */
    ELEMENT *restrict tree = fenwick->tree;
    int64_t count = fenwick->count;

    check_index(index, count, line);

    for (int64_t i = index + 1; i <= count; i += i & -i)
    {
        tree[i] += amount;
    }
}

/** Soma das posições [0, end). */
static ELEMENT FN(_prefix)(FENWICK *fenwick, int64_t end)
{
    ELEMENT total = 0;
    const ELEMENT *restrict tree = fenwick->tree;

    for (int64_t i = end; i > 0; i -= i & -i)
    {
        total += tree[i];
    }

    return total;
}

/** Soma das posições [low, high). */
static ELEMENT FN(_sum)(FENWICK *fenwick, int64_t low, int64_t high, int line)
{
    if (ALG_CHECK && (low < 0 || high < low || high > fenwick->count))
    {
        runtime_error(line, "soma(%lld, %lld) fora de 0..%lld", (long long)low, (long long)high,
                      (long long)fenwick->count);
    }

    return FN(_prefix)(fenwick, high) - FN(_prefix)(fenwick, low);
}
