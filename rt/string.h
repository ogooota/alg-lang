/* Texto: vetor de bytes (ArrayU8) com leitura, escrita, comparação, hash e junção.
 * Entra no C gerado depois do template de ArrayU8 (veja gen.c). */

/** String nova com uma cópia do texto (literal do programa, número convertido). */
static ArrayU8 str_from(const char *text, int64_t length)
{
    return array_u8_lit(length, (const uint8_t *)text);
}

/** Escreve o texto, sem separador. */
static void str_write(ArrayU8 text)
{
    write_raw((const char *)text.data, (size_t)text.length);
}

/** Escreve um caractere. */
static void write_char(int64_t character)
{
    char byte = (char)character;
    write_raw(&byte, 1);
}

/** Lê uma palavra (até o próximo espaço); copia de uma vez quando ela está inteira no buffer. */
static ArrayU8 str_read(int line)
{
    skip_spaces(line);
    input_after_token = 1;

    const char *start = input_buffer + input_position;
    const char *end = input_buffer + input_length;
    const char *cursor = start;

    /* aqui o limite é necessário: o sentinela '!' é maior que espaço */
    while (cursor < end && (unsigned char)*cursor > ' ')
    {
        cursor++;
    }

    if (cursor < end)
    {
        input_position = (size_t)(cursor - input_buffer);
        return str_from(start, cursor - start);
    }

    /* a palavra continua no próximo bloco da entrada: caractere a caractere */
    ArrayU8 word = array_u8_lit(0, NULL);
    int next_char = peek_input();

    while (next_char > ' ')
    {
        array_u8_push(&word, (uint8_t)next_char, line);
        input_position++;
        next_char = peek_input();
    }

    return word;
}

/** Lê a linha inteira (sem o fim de linha). Logo depois de ler um token, pula o resto vazio da
 * linha. */
static ArrayU8 str_read_line(int line)
{
    int next_char = peek_input();

    if (input_after_token)
    {
        input_after_token = 0;

        while (next_char == ' ' || next_char == '\t' || next_char == '\r')
        {
            input_position++;
            next_char = peek_input();
        }

        if (next_char == '\n')
        {
            input_position++;
            next_char = peek_input();
        }
    }

    if (next_char < 0)
    {
        runtime_error(line, "a entrada acabou antes do esperado");
    }

    ArrayU8 text = array_u8_lit(0, NULL);

    while (next_char >= 0 && next_char != '\n')
    {
        if (next_char != '\r')
        {
            array_u8_push(&text, (uint8_t)next_char, line);
        }

        input_position++;
        next_char = peek_input();
    }

    if (next_char == '\n')
    {
        input_position++;
    }

    return text;
}

/** Ordem de dicionário por byte; o prefixo vem antes. */
static inline int str_less(ArrayU8 left, ArrayU8 right)
{
    int64_t shortest = left.length < right.length ? left.length : right.length;
    int order = shortest ? memcmp(left.data, right.data, (size_t)shortest) : 0;
    return order < 0 || (order == 0 && left.length < right.length);
}

static inline int str_equal(ArrayU8 left, ArrayU8 right)
{
    return left.length == right.length &&
           (!left.length || memcmp(left.data, right.data, (size_t)left.length) == 0);
}

/** Hash do texto (FNV-1a) misturado com a semente da execução. */
static inline uint64_t text_hash(ArrayU8 text)
{
    uint64_t hash = 1469598103934665603ULL;

    for (int64_t i = 0; i < text.length; i++)
    {
        hash = (hash ^ text.data[i]) * 1099511628211ULL;
    }

    return mix_bits(hash ^ hash_seed);
}

/** left + right: string nova. */
static ArrayU8 str_concat(ArrayU8 left, ArrayU8 right)
{
    ArrayU8 result = array_u8_lit(left.length, left.data);

    for (int64_t i = 0; i < right.length; i++)
    {
        array_u8_push(&result, right.data[i], 0);
    }

    return result;
}

/** text + caractere: string nova. */
static ArrayU8 str_concat_char(ArrayU8 text, int64_t character)
{
    ArrayU8 result = array_u8_lit(text.length, text.data);
    array_u8_push(&result, (uint8_t)character, 0);
    return result;
}

/** Acrescenta o texto extra no fim de text (no lugar). */
static void str_append(ArrayU8 *text, ArrayU8 extra, int line)
{
    for (int64_t i = 0; i < extra.length; i++)
    {
        array_u8_push(text, extra.data[i], line);
    }
}

/** str(n) para inteiro. */
static ArrayU8 str_from_int(int64_t number)
{
    char digits[24];
    int length = snprintf(digits, sizeof digits, "%lld", (long long)number);
    return str_from(digits, length);
}

/** str(x) para real (15 algarismos significativos, como o write). */
static ArrayU8 str_from_real(double number)
{
    char digits[40];
    int length = snprintf(digits, sizeof digits, "%.15g", number);
    return str_from(digits, length);
}

/* ---------- string guardada (em vetor, heap, fila, mapa) ----------
 * 16 bytes, como as strings do Umbra (Neumann e Freitag, CIDR 2020): tamanho, 4 primeiros
 * bytes e ponteiro. Comparar tamanho + prefixo é um load só e quase sempre decide sem seguir o ponteiro. */
typedef struct
{
    uint32_t length;
    uint8_t prefix[4];
    uint8_t *data;
} Str;

/** Guarda a string usando os dados dela (sem copiar): só para strings recém-criadas. */
static inline Str str_adopt(ArrayU8 text)
{
    Str packed = {(uint32_t)text.length, {0}, text.data};
    memcpy(packed.prefix, text.data, text.length < 4 ? (size_t)text.length : 4);
    return packed;
}

/** Guarda uma cópia da string: mudar a original depois não muda a guardada. */
static inline Str str_pack(ArrayU8 text)
{
    return str_adopt(array_u8_copy(text));
}

/** A string guardada como valor; o primeiro add realoca, então quem leu pode crescer a sua cópia. */
static inline ArrayU8 str_unpack(Str packed)
{
    ArrayU8 text = {0};
    text.data = packed.data;
    text.length = packed.length;
    text.capacity = packed.length;
    text.stride = 1;
    text.dimension_count = 1;
    return text;
}

static inline uint64_t str_head(Str packed)
{
    uint64_t head;
    memcpy(&head, &packed, sizeof head);
    return head;
}

static inline int packed_equal(Str left, Str right)
{
    return str_head(left) == str_head(right) &&
           (left.length <= 4 || memcmp(left.data + 4, right.data + 4, left.length - 4) == 0);
}

/** Guardada == procurada: tamanho e prefixo primeiro, sem tocar nos dados guardados. */
static inline int packed_equal_text(Str stored, ArrayU8 text)
{
    if (stored.length != (uint64_t)text.length)
    {
        return 0;
    }

    size_t head = stored.length < 4 ? stored.length : 4;
    return memcmp(stored.prefix, text.data, head) == 0 &&
           (stored.length <= 4 || memcmp(stored.data + 4, text.data + 4, stored.length - 4) == 0);
}

/** Ordem de dicionário: o prefixo (com zeros depois do fim) decide quando difere. */
static inline int packed_less(Str left, Str right)
{
    int order = memcmp(left.prefix, right.prefix, 4);

    if (order)
    {
        return order < 0;
    }

    return str_less(str_unpack(left), str_unpack(right));
}

static inline uint64_t packed_hash(Str packed)
{
    return text_hash(str_unpack(packed));
}

/** Lê uma palavra direto para uma posição de estrutura (a palavra lida já é nova: sem cópia). */
static Str packed_read(int line)
{
    return str_adopt(str_read(line));
}

/* 8 primeiros bytes em big-endian, com zero depois do fim: a ordem do prefixo é a do texto */
static inline uint64_t str_prefix(ArrayU8 text)
{
    uint64_t prefix = 0;
    int64_t count = text.length < 8 ? text.length : 8;

    for (int64_t i = 0; i < count; i++)
    {
        prefix |= (uint64_t)text.data[i] << (56 - 8 * i);
    }

    return prefix;
}

static int str_index_less(uint64_t left, uint64_t right, const void *context)
{
    const Str *texts = context;
    return packed_less(texts[left], texts[right]);
}

/** Ordem de dicionário: radix nos prefixos de 8 bytes, desempate só nos blocos iguais, permutação no lugar. */
static void str_sort(Str *texts, int64_t count)
{
    if (count < 2)
    {
        return;
    }

    OrderKey *keys = malloc((size_t)count * sizeof *keys);

    for (int64_t i = 0; i < count; i++)
    {
        keys[i].prefix = str_prefix(str_unpack(texts[i]));
        keys[i].index = (uint64_t)i;
    }

    order_sort(keys, count, str_index_less, texts);

    /* permuta no lugar seguindo os ciclos: a posição i recebe texts[keys[i].index] */
    for (int64_t i = 0; i < count; i++)
    {
        if (keys[i].index == (uint64_t)i)
        {
            continue;
        }

        Str first = texts[i];
        int64_t position = i;

        while (keys[position].index != (uint64_t)i)
        {
            int64_t source = (int64_t)keys[position].index;
            texts[position] = texts[source];
            keys[position].index = (uint64_t)position;
            position = source;
        }

        texts[position] = first;
        keys[position].index = (uint64_t)position;
    }

    free(keys);
}
