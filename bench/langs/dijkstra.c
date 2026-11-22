#include "io.h"

typedef struct
{
    int64_t distance;
    int32_t vertex;
} Entry;

static Entry *heap;
static int64_t heap_size;

static void push(Entry entry)
{
    int64_t i = heap_size++;

    while (i > 0 && heap[(i - 1) / 2].distance > entry.distance)
    {
        heap[i] = heap[(i - 1) / 2];
        i = (i - 1) / 2;
    }

    heap[i] = entry;
}

static Entry pop(void)
{
    Entry top = heap[0];
    Entry last = heap[--heap_size];
    int64_t i = 0;

    while (2 * i + 1 < heap_size)
    {
        int64_t child = 2 * i + 1;

        if (child + 1 < heap_size && heap[child + 1].distance < heap[child].distance)
        {
            child++;
        }

        if (heap[child].distance >= last.distance)
        {
            break;
        }

        heap[i] = heap[child];
        i = child;
    }

    heap[i] = last;
    return top;
}

int main(void)
{
    int64_t n = read_int();
    int64_t m = read_int();
    int32_t *from = malloc(m * sizeof *from);
    int32_t *to = malloc(m * sizeof *to);
    int64_t *weight = malloc(m * sizeof *weight);
    int64_t *start = calloc(n + 1, sizeof *start);
    int32_t *adjacent = malloc(m * sizeof *adjacent);
    int64_t *adjacent_weight = malloc(m * sizeof *adjacent_weight);
    int64_t *distance = malloc(n * sizeof *distance);

    for (int64_t i = 0; i < m; i++)
    {
        from[i] = (int32_t)read_int();
        to[i] = (int32_t)read_int();
        weight[i] = read_int();
        start[from[i] + 1]++;
    }

    for (int64_t i = 0; i < n; i++)
    {
        start[i + 1] += start[i];
    }

    int64_t *fill = malloc(n * sizeof *fill);
    memcpy(fill, start, n * sizeof *fill);

    for (int64_t i = 0; i < m; i++)
    {
        adjacent[fill[from[i]]] = to[i];
        adjacent_weight[fill[from[i]]++] = weight[i];
    }

    for (int64_t i = 0; i < n; i++)
    {
        distance[i] = INT64_MAX;
    }

    heap = malloc((m + 1) * sizeof *heap);
    distance[0] = 0;
    push((Entry){0, 0});

    while (heap_size)
    {
        Entry entry = pop();

        if (entry.distance > distance[entry.vertex])
        {
            continue;
        }

        for (int64_t e = start[entry.vertex]; e < start[entry.vertex + 1]; e++)
        {
            int64_t candidate = entry.distance + adjacent_weight[e];

            if (candidate < distance[adjacent[e]])
            {
                distance[adjacent[e]] = candidate;
                push((Entry){candidate, adjacent[e]});
            }
        }
    }

    int64_t total = 0;

    for (int64_t i = 0; i < n; i++)
    {
        if (distance[i] != INT64_MAX)
        {
            total += distance[i];
        }
    }

    write_int(total, '\n');
    flush_output();
}
