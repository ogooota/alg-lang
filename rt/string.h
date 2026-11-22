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
static inline uint64_t str_hash(ArrayU8 text)
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

/* chave de ordenação: 8 primeiros bytes em big-endian (zero depois do fim) e o índice */
typedef struct
{
    uint64_t prefix;
    uint64_t index;
} StringKey;

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

/** Ordena em ordem de dicionário: radix nos prefixos de 8 bytes, desempate só nos blocos iguais. */
static void str_sort(ArrayU8 *texts, int64_t count)
{
    if (count < 2)
    {
        return;
    }

    StringKey *keys = malloc((size_t)count * sizeof *keys);
    StringKey *buffer = malloc((size_t)count * sizeof *keys);
    uint64_t all_or = 0;
    uint64_t all_and = ~0ULL;

    for (int64_t i = 0; i < count; i++)
    {
        keys[i].prefix = str_prefix(texts[i]);
        keys[i].index = (uint64_t)i;
        all_or |= keys[i].prefix;
        all_and &= keys[i].prefix;
    }

    /* LSD radix de 8 bits; pula o byte que é igual em todas as chaves */
    for (int shift = 0; shift < 64; shift += 8)
    {
        if ((((all_or ^ all_and) >> shift) & 0xFF) == 0)
        {
            continue;
        }

        int64_t start[256] = {0};

        for (int64_t i = 0; i < count; i++)
        {
            start[(keys[i].prefix >> shift) & 0xFF]++;
        }

        for (int64_t digit = 0, total = 0; digit < 256; digit++)
        {
            int64_t size = start[digit];
            start[digit] = total;
            total += size;
        }

        for (int64_t i = 0; i < count; i++)
        {
            buffer[start[(keys[i].prefix >> shift) & 0xFF]++] = keys[i];
        }

        StringKey *swap = keys;
        keys = buffer;
        buffer = swap;
    }

    /* blocos com o mesmo prefixo: ordem completa por inserção (blocos costumam ser pequenos) */
    for (int64_t low = 0; low < count;)
    {
        int64_t high = low + 1;

        while (high < count && keys[high].prefix == keys[low].prefix)
        {
            high++;
        }

        for (int64_t i = low + 1; i < high; i++)
        {
            StringKey moving = keys[i];
            int64_t j = i;

            while (j > low && str_less(texts[moving.index], texts[keys[j - 1].index]))
            {
                keys[j] = keys[j - 1];
                j--;
            }

            keys[j] = moving;
        }

        low = high;
    }

    /* permuta no lugar seguindo os ciclos: a posição i recebe texts[keys[i].index] */
    for (int64_t i = 0; i < count; i++)
    {
        if (keys[i].index == (uint64_t)i)
        {
            continue;
        }

        ArrayU8 first = texts[i];
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
    free(buffer);
}
