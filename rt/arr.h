/* Template de vetor: o gerador define ELEMENT, VALUE, ARRAY, FN, READ_ELEMENT, WRITE_ELEMENT,
 * LESS, LOAD e KIND antes de incluir (detalhes em docs/compilador.md). */

typedef struct
{
    ELEMENT *data;
    int64_t length;         /* tamanho da 1ª dimensão */
    int64_t capacity;       /* capacidade (1D dinâmico); -1 em views */
    int64_t stride;         /* elementos por linha da 1ª dimensão */
    int64_t inner_sizes[3]; /* tamanhos das dimensões 2..4 */
    int dimension_count;    /* número de dimensões */
} ARRAY;

/** Vetor novo e zerado com até 4 dimensões. */
static ARRAY FN(_new)(int line, int dimension_count, int64_t size0, int64_t size1, int64_t size2,
                      int64_t size3)
{
    ARRAY array = {0};
    int64_t sizes[4] = {size0, size1, size2, size3};

    for (int i = 0; i < dimension_count; i++)
    {
        if (sizes[i] < 0)
        {
            runtime_error(line, "tamanho negativo (%lld) ao criar vetor", (long long)sizes[i]);
        }
    }

    array.dimension_count = dimension_count;
    array.length = size0;
    array.stride = 1;

    for (int i = 1; i < dimension_count; i++)
    {
        array.inner_sizes[i - 1] = sizes[i];
        array.stride *= sizes[i];
    }

    array.capacity = dimension_count == 1 ? size0 : -1;
    array.data = arena_allocate((size_t)(size0 * array.stride) * sizeof(ELEMENT));
    return array;
}

/** Vetor 1D com uma cópia dos valores dados (pode crescer). */
static ARRAY FN(_lit)(int64_t count, const ELEMENT *source)
{
    ARRAY array = {0};

    array.dimension_count = 1;
    array.length = count;
    array.stride = 1;
    array.capacity = count < 4 ? 4 : count;
    array.data = arena_allocate((size_t)array.capacity * sizeof(ELEMENT));

    if (count)
    {
        memcpy(array.data, source, (size_t)count * sizeof(ELEMENT));
    }

    return array;
}

/** Endereço do elemento i (checa o índice). */
static inline ELEMENT *FN(_at)(ARRAY array, int64_t index, int line)
{
    check_index(index, array.length, line);
    return array.data + index;
}

/** Linha i de um vetor de várias dimensões (view, sem cópia). */
static inline ARRAY FN(_row)(ARRAY array, int64_t index, int line)
{
    ARRAY row = {0};

    check_index(index, array.length, line);
    row.data = array.data + index * array.stride;
    row.dimension_count = array.dimension_count - 1;
    row.length = array.inner_sizes[0];
    row.inner_sizes[0] = array.inner_sizes[1];
    row.inner_sizes[1] = array.inner_sizes[2];
    row.stride = 1;

    for (int j = 0; j < row.dimension_count - 1; j++)
    {
        row.stride *= row.inner_sizes[j];
    }

    row.capacity = -1;
    return row;
}

/** Fatia [low, high) da primeira dimensão (view, sem cópia). */
static inline ARRAY FN(_slice)(ARRAY array, int64_t low, int64_t high, int line)
{
    if (ALG_CHECK && (low < 0 || high < low || high > array.length))
    {
        runtime_error(line, "fatia %lld..%lld fora de 0..%lld", (long long)low, (long long)high,
                      (long long)array.length);
    }

    array.data += low * array.stride;
    array.length = high - low;
    array.capacity = -1;
    return array;
}

/** Cópia independente do vetor (ou da fatia). */
static ARRAY FN(_copy)(ARRAY array)
{
    ARRAY copy = array;
    size_t bytes = (size_t)(array.length * array.stride) * sizeof(ELEMENT);

    copy.data = arena_allocate(bytes);

    if (bytes)
    {
        memcpy(copy.data, array.data, bytes);
    }

    copy.capacity = array.dimension_count == 1 ? array.length : -1;
    return copy;
}

/** Acrescenta no fim, dobrando a capacidade quando enche. */
static void FN(_push)(ARRAY *array, ELEMENT item, int line)
{
    /* -2: string emprestada de uma estrutura (str_unpack); crescer já faz a cópia própria */
    if (array->capacity < 0 && array->capacity != -2)
    {
        runtime_error(line, "não dá para adicionar numa fatia ou num vetor de várias dimensões");
    }

    if (array->length == array->capacity || array->capacity == -2)
    {
        int64_t cap = array->length < 4 ? 8 : array->length * 2;
        ELEMENT *data = arena_allocate((size_t)cap * sizeof(ELEMENT));

        if (array->length)
        {
            memcpy(data, array->data, (size_t)array->length * sizeof(ELEMENT));
        }

        array->data = data;
        array->capacity = cap;
    }

    if (array->dimension_count == 0)
    {
        array->dimension_count = 1;
        array->stride = 1;
    }

    array->data[array->length] = item;
    array->length++;
}

/** Tira e devolve o último elemento. */
static VALUE FN(_pop)(ARRAY *array, int line)
{
    if (array->length == 0)
    {
        runtime_error(line, "remover_ultimo em vetor vazio");
    }

    if (array->capacity < 0 && array->capacity != -2)
    {
        runtime_error(line, "não dá para remover de uma fatia");
    }

    array->length--;
    return LOAD(array->data[array->length]);
}

#if KIND < 2

/** Soma dos elementos. */
static VALUE FN(_sum)(ARRAY array, int line)
{
    VALUE total = 0;

    (void)line;

    for (int64_t i = 0; i < array.length; i++)
    {
        total += LOAD(array.data[i]);
    }

    return total;
}
#endif

/** Menor elemento (erro se vazio). */
static VALUE FN(_min)(ARRAY array, int line)
{
    if (array.length == 0)
    {
        runtime_error(line, "min de vetor vazio");
    }

    ELEMENT best = array.data[0];

    for (int64_t i = 1; i < array.length; i++)
    {
        if (LESS(array.data[i], best))
        {
            best = array.data[i];
        }
    }

    return LOAD(best);
}

/** Maior elemento (erro se vazio). */
static VALUE FN(_max)(ARRAY array, int line)
{
    if (array.length == 0)
    {
        runtime_error(line, "max de vetor vazio");
    }

    ELEMENT best = array.data[0];

    for (int64_t i = 1; i < array.length; i++)
    {
        if (LESS(best, array.data[i]))
        {
            best = array.data[i];
        }
    }

    return LOAD(best);
}

/* lê todos os elementos (ler(a) com 'a' já existente) */
static void FN(_read)(ARRAY array, int line)
{
    int64_t total = array.length * array.stride;

    for (int64_t i = 0; i < total; i++)
    {
        array.data[i] = READ_ELEMENT(line);
    }
}

/* 1D: elementos separados por espaço; 2D ou mais: uma linha por linha */
static void FN(_write)(ARRAY array)
{
    for (int64_t i = 0; i < array.length; i++)
    {
        if (array.dimension_count <= 1)
        {
            if (i)
            {
                write_space();
            }

            WRITE_ELEMENT(LOAD(array.data[i]));
        }
        else
        {
            if (i)
            {
                write_newline();
            }

            FN(_write)(FN(_row)(array, i, 0));
        }
    }
}

/** Guarda o valor num espaço temporário e devolve o endereço (para passar view a função). */
static ARRAY *FN(_tmp)(ARRAY *slot, ARRAY value)
{
    *slot = value;
    return slot;
}

/* ---------- ordenação ---------- */

static void FN(_insertion)(ELEMENT *data, int64_t count)
{
    for (int64_t i = 1; i < count; i++)
    {
        ELEMENT item = data[i];
        int64_t j = i - 1;

        while (j >= 0 && LESS(item, data[j]))
        {
            data[j + 1] = data[j];
            j--;
        }

        data[j + 1] = item;
    }
}

/** Desce o elemento do nó até o heap da ordenação ficar válido. */
static void FN(_sift)(ELEMENT *data, int64_t count, int64_t node)
{
    ELEMENT item = data[node];

    for (;;)
    {
        int64_t child = 2 * node + 1;

        if (child >= count)
        {
            break;
        }

        if (child + 1 < count && LESS(data[child], data[child + 1]))
        {
            child++;
        }

        if (!LESS(item, data[child]))
        {
            break;
        }

        data[node] = data[child];
        node = child;
    }

    data[node] = item;
}

/** Heapsort: o plano B do introsort quando a recursão afunda. */
static void FN(_heapsort)(ELEMENT *data, int64_t count)
{
    for (int64_t i = count / 2 - 1; i >= 0; i--)
    {
        FN(_sift)(data, count, i);
    }

    for (int64_t i = count - 1; i > 0; i--)
    {
        ELEMENT swap = data[0];
        data[0] = data[i];
        data[i] = swap;
        FN(_sift)(data, i, 0);
    }
}

/*
 * Introsort: quicksort com pivô = mediana de três (um deles sorteado, contra
 * entrada adversária), heapsort se a recursão afundar demais e inserção nos
 * pedaços pequenos. Partição de Hoare: [0, j] <= pivô <= [j + 1, n).
 */
static void FN(_intro)(ELEMENT *data, int64_t count, int depth)
{
    while (count > 16)
    {
        if (depth == 0)
        {
            FN(_heapsort)(data, count);
            return;
        }

        depth--;

        int64_t mid = (count - 1) / 2;
        int64_t random_index = (int64_t)(random_u64() % (uint64_t)count);
        ELEMENT sample_a = data[random_index];
        ELEMENT sample_b = data[mid];
        ELEMENT sample_c = data[count - 1];
        ELEMENT pivot;

        if (LESS(sample_a, sample_b))
        {
            pivot = LESS(sample_b, sample_c) ? sample_b
                                             : (LESS(sample_a, sample_c) ? sample_c : sample_a);
        }
        else
        {
            pivot = LESS(sample_a, sample_c) ? sample_a
                                             : (LESS(sample_b, sample_c) ? sample_c : sample_b);
        }

        int64_t left = -1;
        int64_t right = count;

        for (;;)
        {
            do
            {
                left++;
            } while (LESS(data[left], pivot));

            do
            {
                right--;
            } while (LESS(pivot, data[right]));

            if (left >= right)
            {
                break;
            }

            ELEMENT swap = data[left];
            data[left] = data[right];
            data[right] = swap;
        }

        /* recursão no lado menor, laço no maior: pilha O(log n) */
        int64_t left_count = right + 1;

        if (left_count < count - left_count)
        {
            FN(_intro)(data, left_count, depth);
            data += left_count;
            count -= left_count;
        }
        else
        {
            FN(_intro)(data + left_count, count - left_count, depth);
            count = left_count;
        }
    }

    FN(_insertion)(data, count);
}

#if KIND == 0

/* Radix sort LSD por bytes; tipo com sinal inverte o bit de sinal da sua largura.
 * Bytes iguais em todos os elementos são pulados (veja docs/compilador.md). */
static ELEMENT *FN(_scratch);
static int64_t FN(_scratch_cap);

/** Chave sem sinal do elemento, na ordem certa para o radix sort. */
static inline uint64_t FN(_key)(ELEMENT item)
{
    const int bits = 8 * (int)sizeof(ELEMENT);
    const uint64_t mask = bits == 64 ? ~0ULL : (1ULL << bits) - 1;
    uint64_t key = (uint64_t)(int64_t)item & mask;

    if ((ELEMENT)-1 < (ELEMENT)0)
    {
        key ^= 1ULL << (bits - 1);
    }

    return key;
}

/** Radix sort LSD por bytes, pulando os bytes iguais em todos os elementos. */
static void FN(_radix)(ELEMENT *data, int64_t count)
{
    static int64_t counts[8][256];
    int bytes = (int)sizeof(ELEMENT);

    if (FN(_scratch_cap) < count)
    {
        FN(_scratch) = grow_memory(FN(_scratch), (size_t)count * sizeof(ELEMENT));
        FN(_scratch_cap) = count;
    }

    memset(counts, 0, sizeof counts);

    for (int64_t i = 0; i < count; i++)
    {
        uint64_t key = FN(_key)(data[i]);

        for (int byte = 0; byte < bytes; byte++)
        {
            counts[byte][(key >> (8 * byte)) & 255]++;
        }
    }

    ELEMENT *source = data;
    ELEMENT *destination = FN(_scratch);

    for (int byte = 0; byte < bytes; byte++)
    {
        int64_t *byte_counts = counts[byte];
        int trivial = 0;

        for (int bucket = 0; bucket < 256; bucket++)
        {
            if (byte_counts[bucket] == count)
            {
                trivial = 1;
            }
        }

        if (trivial)
        {
            continue;
        }

        int64_t pos = 0;

        for (int bucket = 0; bucket < 256; bucket++)
        {
            int64_t key = byte_counts[bucket];
            byte_counts[bucket] = pos;
            pos += key;
        }

        for (int64_t i = 0; i < count; i++)
        {
            ELEMENT item = source[i];
            destination[byte_counts[(FN(_key)(item) >> (8 * byte)) & 255]++] = item;
        }

        ELEMENT *swap = source;
        source = destination;
        destination = swap;
    }

    if (source != data)
    {
        memcpy(data, source, (size_t)count * sizeof(ELEMENT));
    }
}

#endif

/** Ordena o vetor 1D (radix para inteiros, introsort para o resto); maior primeiro se pedido. */
static void FN(_sort)(ARRAY array, int descending, int line)
{
    if (ALG_CHECK && array.dimension_count > 1)
    {
        runtime_error(line, "ordenar() funciona com vetor 1D");
    }

    int depth = 2;

    for (int64_t size = array.length; size > 1; size >>= 1)
    {
        depth += 2;
    }

#if KIND == 0
    if (array.length > 256)
    {
        FN(_radix)(array.data, array.length);
    }
    else
    {
        FN(_intro)(array.data, array.length, depth);
    }
#elif KIND == 3
    str_sort(array.data, array.length);
#else
    FN(_intro)(array.data, array.length, depth);
#endif

    if (descending)
    {
        for (int64_t i = 0, j = array.length - 1; i < j; i++, j--)
        {
            ELEMENT swap = array.data[i];
            array.data[i] = array.data[j];
            array.data[j] = swap;
        }
    }
}
