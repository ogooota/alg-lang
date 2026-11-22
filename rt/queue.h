/*
 * Template de fila (buffer circular). O gerador define T, Q e F(x).
 *
 * Invariante: os elementos são d[head], d[head + 1], ... (módulo cap),
 * n deles; cap é potência de 2, então "módulo" é um & (cap - 1).
 */

typedef struct
{
    ELEMENT *data;
    int64_t head;
    int64_t count;
    int64_t capacity;
} QUEUE;

/** Fila vazia. */
static QUEUE *FN(_new)(void)
{
    return arena_allocate(sizeof(QUEUE));
}

/** Acrescenta no fim da fila. */
static void FN(_push)(QUEUE *queue, ELEMENT item)
{
    if (queue->count == queue->capacity)
    {
        int64_t capacity = queue->capacity ? queue->capacity * 2 : 16;
        ELEMENT *data = grow_memory(NULL, (size_t)capacity * sizeof(ELEMENT));

        /* desenrola o círculo no começo do bloco novo */
        for (int64_t i = 0; i < queue->count; i++)
        {
            data[i] = queue->data[(queue->head + i) & (queue->capacity - 1)];
        }

        free(queue->data);
        queue->data = data;
        queue->head = 0;
        queue->capacity = capacity;
    }

    queue->data[(queue->head + queue->count) & (queue->capacity - 1)] = item;
    queue->count++;
}

/** Primeiro elemento da fila, sem tirar. */
static ELEMENT FN(_front)(QUEUE *queue, int line)
{
    if (queue->count == 0)
    {
        runtime_error(line, "frente() de fila vazia");
    }

    return queue->data[queue->head];
}

/** Tira e devolve o primeiro da fila. */
static ELEMENT FN(_pop)(QUEUE *queue, int line)
{
    if (queue->count == 0)
    {
        runtime_error(line, "desenfileirar() de fila vazia");
    }

    ELEMENT item = queue->data[queue->head];
    queue->head = (queue->head + 1) & (queue->capacity - 1);
    queue->count--;
    return item;
}
