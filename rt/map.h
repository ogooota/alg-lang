/* Template de mapa: hash com endereçamento aberto e sondagem linear (KEY, KEY_INPUT, MAP_VALUE,
 * MAP, FN, HASH, HASH_STORED, KEYS_EQUAL, KEY_STORE, KEY_LESS, ORDER_PREFIX opcional). Detalhes em docs/compilador.md. */

/* chave e valor juntos: a busca que acha a chave já trouxe o valor para o cache */
typedef struct
{
    KEY key;
    MAP_VALUE value;
} FN(_Slot);

/* controle de cada posição: 0 vazia, 1 removida, 0x80 | 7 bits do hash quando ocupada */
enum
{
    FN(_EMPTY) = 0,
    FN(_DELETED) = 1
};

typedef struct
{
    FN(_Slot) *slots;
    uint8_t *controls;
    int64_t capacity;
    int64_t count;
    int64_t used_slots;
    int64_t writes; /* conta _ref e _del: se não mudou, a foto do for ainda vale */
} MAP;

static inline uint8_t FN(_tag)(uint64_t hash)
{
    return (uint8_t)(0x80 | (hash >> 57));
}

/** Aloca as tabelas do mapa com a capacidade dada. */
static void FN(_alloc)(MAP *map, int64_t capacity)
{
    map->slots = grow_memory(NULL, (size_t)capacity * sizeof(FN(_Slot)));
    map->controls = calloc((size_t)capacity, 1);

    if (!map->controls)
    {
        runtime_error(0, "memória esgotada");
    }

    map->capacity = capacity;
    map->count = 0;
    map->used_slots = 0;
    map->writes++;
}

/** Mapa vazio. */
static MAP *FN(_new)(void)
{
    MAP *map = arena_allocate(sizeof(MAP));
    FN(_alloc)(map, 16);
    return map;
}

/* posição da chave, ou -1; só compara a chave quando os 7 bits do hash batem */
static int64_t FN(_find_hashed)(MAP *map, KEY_INPUT key, uint64_t hash)
{
    uint64_t mask = (uint64_t)map->capacity - 1;
    uint64_t slot = hash & mask;
    uint8_t tag = FN(_tag)(hash);

    while (map->controls[slot] != FN(_EMPTY))
    {
        if (map->controls[slot] == tag && KEYS_EQUAL(map->slots[slot].key, key))
        {
            return (int64_t)slot;
        }

        slot = (slot + 1) & mask;
    }

    return -1;
}

static int64_t FN(_find)(MAP *map, KEY_INPUT key)
{
    return FN(_find_hashed)(map, key, HASH(key));
}

/** Reconstrói a tabela com outra capacidade (some com os removidos). */
static void FN(_rehash)(MAP *map, int64_t capacity)
{
    MAP previous = *map;
    uint64_t mask = (uint64_t)capacity - 1;

    FN(_alloc)(map, capacity);

    for (int64_t i = 0; i < previous.capacity; i++)
    {
        if (previous.controls[i] < 0x80)
        {
            continue;
        }

        uint64_t hash = HASH_STORED(previous.slots[i].key);
        uint64_t slot = hash & mask;

        while (map->controls[slot] != FN(_EMPTY))
        {
            slot = (slot + 1) & mask;
        }

        map->controls[slot] = FN(_tag)(hash);
        map->slots[slot] = previous.slots[i];
        map->count++;
        map->used_slots++;
    }

    free(previous.slots);
    free(previous.controls);
}

/* lugar do valor da chave; cria com valor zero se não existir */
static MAP_VALUE *FN(_ref)(MAP *map, KEY_INPUT key)
{
    uint64_t hash = HASH(key);
    int64_t found = FN(_find_hashed)(map, key, hash);

    map->writes++;

    if (found >= 0)
    {
        return &map->slots[found].value;
    }

    if ((map->used_slots + 1) * 2 > map->capacity)
    {
        FN(_rehash)(map, map->count * 4 >= map->capacity ? map->capacity * 2 : map->capacity);
    }

    uint64_t mask = (uint64_t)map->capacity - 1;
    uint64_t slot = hash & mask;

    while (map->controls[slot] >= 0x80)
    {
        slot = (slot + 1) & mask;
    }

    if (map->controls[slot] == FN(_EMPTY))
    {
        map->used_slots++;
    }

    map->controls[slot] = FN(_tag)(hash);
    map->slots[slot].key = KEY_STORE(key);
    memset(&map->slots[slot].value, 0, sizeof(MAP_VALUE));
    map->count++;
    return &map->slots[slot].value;
}

/** Valor da chave, ou zero se não existe (sem criar). */
static MAP_VALUE FN(_get)(MAP *map, KEY_INPUT key)
{
    int64_t found = FN(_find)(map, key);

    if (found >= 0)
    {
        return map->slots[found].value;
    }

    MAP_VALUE zero;
    memset(&zero, 0, sizeof zero);
    return zero;
}

/** Diz se a chave existe. */
static int64_t FN(_has)(MAP *map, KEY_INPUT key)
{
    return FN(_find)(map, key) >= 0;
}

/** Remove a chave (se existir). */
static void FN(_del)(MAP *map, KEY_INPUT key)
{
    int64_t found = FN(_find)(map, key);

    if (found >= 0)
    {
        map->controls[found] = FN(_DELETED);
        map->count--;
        map->writes++;
    }
}

/* foto ordenada dos pares (chave, valor) para o for: percorrer nunca é afetado por mudanças no mapa */
typedef struct
{
    FN(_Slot) *data;
    int64_t length;
    int64_t writes;
} FN(_Entries);

static int FN(_entry_less)(uint64_t left, uint64_t right, const void *context)
{
    const FN(_Slot) *entries = context;
    return KEY_LESS(entries[left].key, entries[right].key);
}

static FN(_Entries) FN(_entries)(MAP *map)
{
    FN(_Entries) entries = {arena_allocate((size_t)map->count * sizeof(FN(_Slot))), map->count,
                            map->writes};
    OrderKey *order = malloc((size_t)map->count * sizeof *order);
    int64_t next = 0;

    for (int64_t i = 0; i < map->capacity; i++)
    {
        if (map->controls[i] >= 0x80)
        {
#ifdef ORDER_PREFIX
            order[next].prefix = ORDER_PREFIX(map->slots[i].key);
#else
            order[next].prefix = 0;
#endif
            order[next].index = (uint64_t)i;
            next++;
        }
    }

    /* o desempate compara direto nas posições da tabela; depois copia na ordem */
    order_sort(order, next, FN(_entry_less), map->slots);

    for (int64_t i = 0; i < next; i++)
    {
        entries.data[i] = map->slots[order[i].index];
    }

    free(order);
    return entries;
}
