/* Template de grafo (WEIGHT, GRAPH, FN): arestas numa lista, viram adjacência compacta (CSR)
 * na primeira consulta a vizinhos (detalhes em docs/compilador.md). */

typedef struct
{
    int64_t vertex_count;
    int directed;
    int weighted;
    int64_t edge_count;
    int64_t edge_capacity;
    int32_t *edge_from;
    int32_t *edge_to;
    WEIGHT *edge_weight;
    int is_built;
    int64_t *adjacency_start;
    int32_t *adjacency;
    WEIGHT *adjacency_weight;
} GRAPH;

/** Grafo com n vértices e nenhuma aresta. */
static GRAPH *FN(_new)(int64_t vertex_count, int directed, int weighted, int line)
{
    if (vertex_count < 0 || vertex_count > INT32_MAX)
    {
        runtime_error(line, "grafo com %lld vértices", (long long)vertex_count);
    }

    GRAPH *graph = arena_allocate(sizeof(GRAPH));
    graph->vertex_count = vertex_count;
    graph->directed = directed;
    graph->weighted = weighted;
    return graph;
}

/** Acrescenta uma aresta (com peso, se o grafo tem pesos). */
static void FN(_edge)(GRAPH *graph, int64_t from, int64_t to, WEIGHT weight, int line)
{
    check_index(from, graph->vertex_count, line);
    check_index(to, graph->vertex_count, line);

    if (graph->edge_count == graph->edge_capacity)
    {
        graph->edge_capacity = graph->edge_capacity ? graph->edge_capacity * 2 : 16;
        graph->edge_from =
            grow_memory(graph->edge_from, (size_t)graph->edge_capacity * sizeof(int32_t));
        graph->edge_to =
            grow_memory(graph->edge_to, (size_t)graph->edge_capacity * sizeof(int32_t));

        if (graph->weighted)
        {
            graph->edge_weight =
                grow_memory(graph->edge_weight, (size_t)graph->edge_capacity * sizeof(WEIGHT));
        }
    }

    graph->edge_from[graph->edge_count] = (int32_t)from;
    graph->edge_to[graph->edge_count] = (int32_t)to;

    if (graph->weighted)
    {
        graph->edge_weight[graph->edge_count] = weight;
    }

    graph->edge_count++;
    graph->is_built = 0;
}

/** Monta a adjacência compacta (CSR), se mudou desde a última vez. */
static void FN(_ready)(GRAPH *graph)
{
    if (graph->is_built)
    {
        return;
    }

    int64_t slot_count = graph->directed ? graph->edge_count : 2 * graph->edge_count;
    int64_t *start = arena_allocate((size_t)(graph->vertex_count + 1) * sizeof(int64_t));
    int32_t *adjacency = arena_allocate((size_t)slot_count * sizeof(int32_t));
    WEIGHT *weights = graph->weighted ? arena_allocate((size_t)slot_count * sizeof(WEIGHT)) : NULL;

    for (int64_t edge = 0; edge < graph->edge_count; edge++)
    {
        start[graph->edge_from[edge] + 1]++;

        if (!graph->directed)
        {
            start[graph->edge_to[edge] + 1]++;
        }
    }

    for (int64_t from = 0; from < graph->vertex_count; from++)
    {
        start[from + 1] += start[from];
    }

    int64_t *next_slot = grow_memory(NULL, (size_t)(graph->vertex_count + 1) * sizeof(int64_t));
    memcpy(next_slot, start, (size_t)(graph->vertex_count + 1) * sizeof(int64_t));

    for (int64_t edge = 0; edge < graph->edge_count; edge++)
    {
        int32_t from = graph->edge_from[edge];
        int32_t to = graph->edge_to[edge];
        int64_t slot = next_slot[from];

        adjacency[slot] = to;
        next_slot[from]++;

        if (weights)
        {
            weights[slot] = graph->edge_weight[edge];
        }

        if (!graph->directed)
        {
            slot = next_slot[to];
            adjacency[slot] = from;
            next_slot[to]++;

            if (weights)
            {
                weights[slot] = graph->edge_weight[edge];
            }
        }
    }

    free(next_slot);
    graph->adjacency_start = start;
    graph->adjacency = adjacency;
    graph->adjacency_weight = weights;
    graph->is_built = 1;
}
