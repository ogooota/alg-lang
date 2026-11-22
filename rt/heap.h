/* Template de heap binário (ELEMENT, HEAP, FN, LESS); sobe e desce com "buraco",
 * como o std::priority_queue (detalhes em docs/compilador.md). */

typedef struct
{
    ELEMENT *data;
    int64_t count;
    int64_t capacity;
} HEAP;

/** Heap vazio. */
static HEAP *FN(_new)(void)
{
    return arena_allocate(sizeof(HEAP));
}

/** Insere mantendo o heap (sobe com buraco). */
static void FN(_push)(HEAP *heap, ELEMENT item)
{
    if (heap->count == heap->capacity)
    {
        heap->capacity = heap->capacity ? heap->capacity * 2 : 16;
        heap->data = grow_memory(heap->data, (size_t)heap->capacity * sizeof(ELEMENT));
    }

    int64_t hole = heap->count;
    heap->count++;

    while (hole > 0)
    {
        int64_t parent = (hole - 1) / 2;

        if (!LESS(item, heap->data[parent]))
        {
            break;
        }

        heap->data[hole] = heap->data[parent];
        hole = parent;
    }

    heap->data[hole] = item;
}

/** Primeiro elemento do heap, sem tirar. */
static ELEMENT FN(_top)(HEAP *heap, int line)
{
    if (heap->count == 0)
    {
        runtime_error(line, "topo() de heap vazio");
    }

    return heap->data[0];
}

/** Tira e devolve o primeiro do heap (desce com buraco). */
static ELEMENT FN(_pop)(HEAP *heap, int line)
{
    if (heap->count == 0)
    {
        runtime_error(line, "remover() de heap vazio");
    }

    ELEMENT first = heap->data[0];
    heap->count--;

    if (heap->count == 0)
    {
        return first;
    }

    ELEMENT last = heap->data[heap->count];
    int64_t hole = 0;

    for (;;)
    {
        int64_t child = 2 * hole + 1;

        if (child >= heap->count)
        {
            break;
        }

        if (child + 1 < heap->count && LESS(heap->data[child + 1], heap->data[child]))
        {
            child++;
        }

        if (!LESS(heap->data[child], last))
        {
            break;
        }

        heap->data[hole] = heap->data[child];
        hole = child;
    }

    heap->data[hole] = last;
    return first;
}
