/* Template de mapa: hash com endereçamento aberto e sondagem linear (KEY, MAP_VALUE, MAP,
 * FN, HASH, KEYS_EQUAL, KEY_ARRAY, KEY_ARRAY_FN). Invariantes em docs/compilador.md. */

typedef struct
{
    KEY *keys;
    MAP_VALUE *values;
    uint8_t *states;
    int64_t capacity;
    int64_t count;
    int64_t used_slots;
} MAP;

/** Aloca as tabelas do mapa com a capacidade dada. */
static void FN(_alloc)(MAP *map, int64_t capacity)
{
    map->keys = grow_memory(NULL, (size_t)capacity * sizeof(KEY));
    map->values = grow_memory(NULL, (size_t)capacity * sizeof(MAP_VALUE));
    map->states = calloc((size_t)capacity, 1);

    if (!map->states)
    {
        runtime_error(0, "memória esgotada");
    }

    map->capacity = capacity;
    map->count = 0;
    map->used_slots = 0;
}

/** Mapa vazio. */
static MAP *FN(_new)(void)
{
    MAP *map = arena_allocate(sizeof(MAP));
    FN(_alloc)(map, 16);
    return map;
}

/* posição da chave, ou -1 */
static int64_t FN(_find)(MAP *map, KEY key)
{
    uint64_t mask = (uint64_t)map->capacity - 1;
    uint64_t slot = HASH(key) & mask;

    while (map->states[slot])
    {
        if (map->states[slot] == 1 && KEYS_EQUAL(map->keys[slot], key))
        {
            return (int64_t)slot;
        }

        slot = (slot + 1) & mask;
    }

    return -1;
}

/** Reconstrói a tabela com outra capacidade (some com os removidos). */
static void FN(_rehash)(MAP *map, int64_t capacity)
{
    MAP previous = *map;

    FN(_alloc)(map, capacity);

    for (int64_t i = 0; i < previous.capacity; i++)
    {
        if (previous.states[i] != 1)
        {
            continue;
        }

        uint64_t mask = (uint64_t)capacity - 1;
        uint64_t slot = HASH(previous.keys[i]) & mask;

        while (map->states[slot])
        {
            slot = (slot + 1) & mask;
        }

        map->states[slot] = 1;
        map->keys[slot] = previous.keys[i];
        map->values[slot] = previous.values[i];
        map->count++;
        map->used_slots++;
    }

    free(previous.keys);
    free(previous.values);
    free(previous.states);
}

/* lugar do valor da chave; cria com valor zero se não existir */
static MAP_VALUE *FN(_ref)(MAP *map, KEY key)
{
    int64_t found = FN(_find)(map, key);

    if (found >= 0)
    {
        return &map->values[found];
    }

    if ((map->used_slots + 1) * 2 > map->capacity)
    {
        FN(_rehash)(map, map->count * 4 >= map->capacity ? map->capacity * 2 : map->capacity);
    }

    uint64_t mask = (uint64_t)map->capacity - 1;
    uint64_t slot = HASH(key) & mask;

    while (map->states[slot] == 1)
    {
        slot = (slot + 1) & mask;
    }

    if (map->states[slot] == 0)
    {
        map->used_slots++;
    }

    map->states[slot] = 1;
    map->keys[slot] = KEY_STORE(key);
    memset(&map->values[slot], 0, sizeof(MAP_VALUE));
    map->count++;
    return &map->values[slot];
}

/** Valor da chave, ou zero se não existe (sem criar). */
static MAP_VALUE FN(_get)(MAP *map, KEY key)
{
    int64_t found = FN(_find)(map, key);

    if (found >= 0)
    {
        return map->values[found];
    }

    MAP_VALUE zero;
    memset(&zero, 0, sizeof zero);
    return zero;
}

/** Diz se a chave existe. */
static int64_t FN(_has)(MAP *map, KEY key)
{
    return FN(_find)(map, key) >= 0;
}

/** Remove a chave (se existir). */
static void FN(_del)(MAP *map, KEY key)
{
    int64_t found = FN(_find)(map, key);

    if (found >= 0)
    {
        map->states[found] = 2;
        map->count--;
    }
}

/* cópia das chaves, ordenada: percorrer o mapa nunca é afetado por mudanças nele */
static KEY_ARRAY FN(_keys)(MAP *map)
{
    KEY_ARRAY keys = KEY_ARRAY_FN(_new)(0, 1, map->count, 0, 0, 0);
    int64_t next = 0;

    for (int64_t i = 0; i < map->capacity; i++)
    {
        if (map->states[i] == 1)
        {
            keys.data[next] = map->keys[i];
            next++;
        }
    }

    KEY_ARRAY_FN(_sort)(keys, 0, 0);
    return keys;
}
