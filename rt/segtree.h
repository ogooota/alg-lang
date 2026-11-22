/* Template de segment tree iterativa: ELEMENT, SEGTREE, FN, COMBINE(left, right), IDENTITY.
 * tree[count + i] é a posição i; tree[k] = COMBINE(tree[2k], tree[2k + 1]). */

typedef struct
{
    ELEMENT *tree;
    int64_t count;
} SEGTREE;

/** Segtree de n posições, todas com o elemento neutro (as folhas são preenchidas depois). */
static SEGTREE *FN(_new)(int64_t count)
{
    SEGTREE *segtree = arena_allocate(sizeof(SEGTREE));
    segtree->tree = arena_allocate((size_t)(2 * count + 1) * sizeof(ELEMENT));
    segtree->count = count;

    for (int64_t i = 0; i < 2 * count + 1; i++)
    {
        segtree->tree[i] = IDENTITY;
    }

    return segtree;
}

/** Calcula os nós internos a partir das folhas. */
static SEGTREE *FN(_build)(SEGTREE *segtree)
{
    for (int64_t k = segtree->count - 1; k > 0; k--)
    {
        segtree->tree[k] = COMBINE(segtree->tree[2 * k], segtree->tree[2 * k + 1]);
    }

    return segtree;
}

/** Troca o valor da posição index e atualiza os ancestrais. */
static void FN(_set)(SEGTREE *segtree, int64_t index, ELEMENT value, int line)
{
    /* cópias locais: escrever em tree não obriga o compilador a reler count */
    ELEMENT *restrict tree = segtree->tree;
    int64_t count = segtree->count;

    check_index(index, count, line);

    int64_t k = index + count;
    tree[k] = value;

    for (k /= 2; k > 0; k /= 2)
    {
        tree[k] = COMBINE(tree[2 * k], tree[2 * k + 1]);
    }
}

/** Combinação das posições [low, high). */
static ELEMENT FN(_query)(SEGTREE *segtree, int64_t low, int64_t high, int line)
{
    if (ALG_CHECK && (low < 0 || high <= low || high > segtree->count))
    {
        runtime_error(line, "consulta(%lld, %lld) inválida (intervalo vazio ou fora de 0..%lld)",
                      (long long)low, (long long)high, (long long)segtree->count);
    }

    ELEMENT left_result = IDENTITY;
    ELEMENT right_result = IDENTITY;
    const ELEMENT *restrict tree = segtree->tree;
    int64_t count = segtree->count;

    for (int64_t l = low + count, r = high + count; l < r; l /= 2, r /= 2)
    {
        if (l & 1)
        {
            left_result = COMBINE(left_result, tree[l]);
            l++;
        }

        if (r & 1)
        {
            r--;
            right_result = COMBINE(tree[r], right_result);
        }
    }

    return COMBINE(left_result, right_result);
}
