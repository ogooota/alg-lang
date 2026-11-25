/* runtime do alg: copiado no começo de todo programa gerado */
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#ifdef ALG_NOCHECK
#define ALG_CHECK 0
#else
#define ALG_CHECK 1
#endif

static void alg_main(void);

/* ordem dos números (as tuplas têm a sua: tup_<sig>_lt) */
#define LESS_THAN(a, b) ((a) < (b))

/* ---------- saída bufferizada ---------- */

static char output_buffer[1 << 16];
static size_t output_length;

/** Escreve a saída acumulada no stdout. */
static void flush_output(void)
{
    fwrite(output_buffer, 1, output_length, stdout);
    fflush(stdout);
    output_length = 0;
}

/** Acumula bytes na saída (esvazia o buffer quando enche). */
static void write_raw(const char *text, size_t length)
{
    if (output_length + length > sizeof output_buffer)
    {
        flush_output();
    }

    if (length > sizeof output_buffer)
    {
        fwrite(text, 1, length, stdout);
        return;
    }

    memcpy(output_buffer + output_length, text, length);
    output_length += length;
}

/** Acumula um texto na saída. */
static void write_text(const char *text)
{
    write_raw(text, strlen(text));
}

/** Acumula um espaço na saída. */
static void write_space(void)
{
    write_raw(" ", 1);
}

/** Acumula uma quebra de linha na saída. */
static void write_newline(void)
{
    write_raw("\n", 1);
}

/** Acumula um inteiro na saída (sem printf, mais rápido). */
static void write_int(int64_t number)
{
    char digits[24];
    int start = 24;
    uint64_t magnitude = number < 0 ? 0 - (uint64_t)number : (uint64_t)number;

    do
    {
        digits[--start] = (char)('0' + magnitude % 10);
        magnitude /= 10;
    } while (magnitude);

    if (number < 0)
    {
        digits[--start] = '-';
    }

    write_raw(digits + start, (size_t)(24 - start));
}

/** Acumula um real na saída (15 algarismos significativos). */
static void write_real(double number)
{
    char digits[40];
    int length = snprintf(digits, sizeof digits, "%.15g", number);
    write_raw(digits, (size_t)length);
}

/* ---------- erros ---------- */

#ifdef ALG_ENGLISH
/* mensagens do runtime em inglês (programa #lang en): o texto em português é a chave */
static const char *const RUNTIME_MESSAGES[][2] = {
    {"%lld não cabe em int32", "%lld does not fit in int32"},
    {"a entrada acabou antes do esperado", "the input ended earlier than expected"},
    {"conjuntos(%lld): tamanho inválido", "dsu(%lld): invalid size"},
    {"consulta(%lld, %lld) inválida (intervalo vazio ou fora de 0..%lld)", "query(%lld, %lld) is invalid (empty range or outside 0..%lld)"},
    {"crivo(%lld): o limite precisa ser >= 0", "sieve(%lld): the limit must be >= 0"},
    {"desenfileirar() de fila vazia", "pop() from an empty queue"},
    {"divisão por zero", "division by zero"},
    {"esperava um número inteiro na entrada, veio '%s'", "expected an integer in the input, got '%s'"},
    {"esperava um número real na entrada, veio '%s'", "expected a real number in the input, got '%s'"},
    {"fatia %lld..%lld fora de 0..%lld", "slice %lld..%lld outside 0..%lld"},
    {"fenwick(%lld): tamanho negativo", "fenwick(%lld): negative size"},
    {"frente() de fila vazia", "front() of an empty queue"},
    {"grafo com %lld vértices", "graph with %lld vertices"},
    {"max de vetor vazio", "max of an empty array"},
    {"memória esgotada", "out of memory"},
    {"min de vetor vazio", "min of an empty array"},
    {"não dá para adicionar numa fatia ou num vetor de várias dimensões", "cannot add to a slice or to a multi-dimensional array"},
    {"não dá para remover de uma fatia", "cannot remove from a slice"},
    {"ordenar() funciona com vetor 1D", "sort() works on a 1D array"},
    {"potmod com expoente negativo (%lld)", "powmod with a negative exponent (%lld)"},
    {"potmod com módulo %lld (precisa ser positivo)", "powmod with modulus %lld (it must be positive)"},
    {"remover() de heap vazio", "pop() from an empty heap"},
    {"remover_ultimo em vetor vazio", "pop() on an empty array"},
    {"resto de divisão por zero", "remainder of division by zero"},
    {"soma(%lld, %lld) fora de 0..%lld", "sum(%lld, %lld) outside 0..%lld"},
    {"tamanho negativo (%lld) ao criar vetor", "negative size (%lld) when creating an array"},
    {"topo() de heap vazio", "top() of an empty heap"},
    {"índice %lld fora de 0..%lld", "index %lld outside 0..%lld"},
};
#endif
__attribute__((noreturn, format(printf, 2, 3))) static void runtime_error(int line,
                                                                          const char *format, ...)
{
    va_list arguments;

    flush_output();
#ifdef ALG_ENGLISH
    fprintf(stderr, "error (line %d): ", line);

    for (size_t i = 0; i < sizeof RUNTIME_MESSAGES / sizeof *RUNTIME_MESSAGES; i++)
    {
        if (!strcmp(RUNTIME_MESSAGES[i][0], format))
        {
            format = RUNTIME_MESSAGES[i][1];
            break;
        }
    }
#else
    fprintf(stderr, "erro (linha %d): ", line);
#endif
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fputc('\n', stderr);
    exit(1);
}

/* ---------- arena: tudo é liberado quando o programa termina ---------- */

static char *arena_cursor;
static size_t arena_remaining;

/** Memória zerada da arena; tudo é liberado quando o programa termina. */
static void *arena_allocate(size_t size)
{
    size = (size + 15) & ~(size_t)15;

    if (size == 0)
    {
        size = 16;
    }

    /* bloco grande começa numa linha de cache: load/store AVX não atravessa duas (medido: 0,80× no laço) */
    size_t padding = size >= 1024 ? (size_t)(-(uintptr_t)arena_cursor & 63) : 0;

    if (size + padding > arena_remaining)
    {
        size_t chunk = size + 64 > (16u << 20) ? size + 64 : (16u << 20);
        arena_cursor = calloc(1, chunk);

        if (!arena_cursor)
        {
            runtime_error(0, "memória esgotada");
        }

        arena_remaining = chunk;
        padding = size >= 1024 ? (size_t)(-(uintptr_t)arena_cursor & 63) : 0;
    }

    arena_cursor += padding;
    arena_remaining -= padding;

    void *block = arena_cursor;
    arena_cursor += size;
    arena_remaining -= size;
    return block;
}

/* ---------- entrada: lida sob demanda, em blocos ---------- */

/*
 * read() devolve assim que há dados (no terminal, a cada Enter), então
 * o programa funciona digitando ou com a entrada vinda de um arquivo.
 */
/* um byte a mais para o sentinela '!' depois dos dados (não é espaço nem dígito) */
static char input_buffer[(1 << 16) + 1] = {'!'};
static size_t input_position;
static size_t input_length;

/* a última leitura foi de um token (número ou palavra): read_line pula o resto vazio da linha */
static int input_after_token;

/* devolve o próximo caractere sem consumir; -1 no fim da entrada */
static int peek_input(void)
{
    if (input_position < input_length)
    {
        return (unsigned char)input_buffer[input_position];
    }

    /* mostra o que já foi escrito antes de parar para esperar a digitação */
    flush_output();

    ssize_t received = read(0, input_buffer, sizeof input_buffer - 1);

    while (received < 0 && errno == EINTR)
    {
        received = read(0, input_buffer, sizeof input_buffer - 1);
    }

    input_position = 0;
    input_length = received > 0 ? (size_t)received : 0;
    input_buffer[input_length] = '!';
    return input_length ? (unsigned char)input_buffer[0] : -1;
}

/** Pula espaços e quebras de linha da entrada; erro se ela acabar. */
static void skip_spaces(int line)
{
    int next_char = peek_input();

    while (next_char >= 0 && next_char <= ' ')
    {
        input_position++;
        next_char = peek_input();
    }

    if (next_char < 0)
    {
        runtime_error(line, "a entrada acabou antes do esperado");
    }
}

/* copia o próximo token (até espaço ou fim) para buf */
static size_t read_token(char *buffer, size_t capacity)
{
    size_t length = 0;
    int next_char = peek_input();

    while (next_char > ' ')
    {
        if (length + 1 < capacity)
        {
            buffer[length] = (char)next_char;
            length++;
        }

        input_position++;
        next_char = peek_input();
    }

    buffer[length] = 0;
    return length;
}

/** Lê um inteiro da entrada; erro se vier outra coisa. */
/** Caminho lento: número na fronteira entre blocos, entrada interativa ou erro. */
__attribute__((noinline)) static int64_t read_int_slow(int line)
{
    input_after_token = 1;

    char token[64];
    char *cursor = token;

    skip_spaces(line);
    read_token(token, sizeof token);

    int negative = *cursor == '-';

    if (negative || *cursor == '+')
    {
        cursor++;
    }

    if (*cursor < '0' || *cursor > '9')
    {
        runtime_error(line, "esperava um número inteiro na entrada, veio '%s'", token);
    }

    uint64_t value = 0;

    while (*cursor >= '0' && *cursor <= '9')
    {
        value = value * 10 + (uint64_t)(*cursor - '0');
        cursor++;
    }

    if (*cursor)
    {
        runtime_error(line, "esperava um número inteiro na entrada, veio '%s'", token);
    }

    return negative ? (int64_t)(0 - value) : (int64_t)value;
}

/** Lê um real da entrada; erro se vier outra coisa. */
/** Lê um inteiro: o caminho comum (número inteiro dentro do buffer) é inline e sem cópia.
 * O sentinela '!' depois dos dados para os dois laços sem testar o fim do buffer. */
static inline int64_t read_int(int line)
{
    const char *cursor = input_buffer + input_position;
    const char *end = input_buffer + input_length;

    while ((unsigned char)*cursor <= ' ')
    {
        cursor++;
    }

    int negative = *cursor == '-';
    cursor += negative;

    if ((unsigned)(*cursor - '0') <= 9 && cursor < end)
    {
        uint64_t value = 0;

        while ((unsigned)(*cursor - '0') <= 9)
        {
            value = value * 10 + (uint64_t)(*cursor - '0');
            cursor++;
        }

        /* terminou antes do fim do buffer e num separador: número completo */
        if (cursor < end && (unsigned char)*cursor <= ' ')
        {
            input_position = (size_t)(cursor - input_buffer);
            input_after_token = 1;
            return negative ? (int64_t)(0 - value) : (int64_t)value;
        }
    }

    return read_int_slow(line);
}

static double read_real(int line)
{
    input_after_token = 1;

    char token[128];
    char *parsed_end;

    skip_spaces(line);
    read_token(token, sizeof token);

    double value = strtod(token, &parsed_end);

    if (parsed_end == token || *parsed_end)
    {
        runtime_error(line, "esperava um número real na entrada, veio '%s'", token);
    }

    return value;
}

/* ---------- aritmética ---------- */

/* divisão euclidiana: o resto nunca é negativo e a == (a/b)*b + a%b */
static inline int64_t euclid_mod(int64_t dividend, int64_t divisor, int line)
{
    if (divisor == 0)
    {
        runtime_error(line, "resto de divisão por zero");
    }

    int64_t remainder = dividend % divisor;

    if (remainder < 0)
    {
        remainder += divisor < 0 ? -divisor : divisor;
    }

    return remainder;
}

/** Divisão inteira que arredonda para baixo (combina com o resto euclidiano). */
static inline int64_t floor_div(int64_t dividend, int64_t divisor, int line)
{
    if (divisor == 0)
    {
        runtime_error(line, "divisão por zero");
    }

    return (dividend - euclid_mod(dividend, divisor, line)) / divisor;
}

/* ': int32': o valor precisa caber (sem -O, é checado) */
static inline int64_t fit_int32(int64_t value, int line)
{
    if (ALG_CHECK && (value < INT32_MIN || value > INT32_MAX))
    {
        runtime_error(line, "%lld não cabe em int32", (long long)value);
    }

    return value;
}

/** Lê um inteiro que precisa caber em int32. */
static int64_t read_int32(int line)
{
    return fit_int32(read_int(line), line);
}

/** Menor de dois inteiros. */
static inline int64_t min_int(int64_t left, int64_t right)
{
    return left < right ? left : right;
}

/** Maior de dois inteiros. */
static inline int64_t max_int(int64_t left, int64_t right)
{
    return left > right ? left : right;
}

/** Menor de dois reais. */
static inline double min_real(double left, double right)
{
    return left < right ? left : right;
}

/** Maior de dois reais. */
static inline double max_real(double left, double right)
{
    return left > right ? left : right;
}

/** Erro com a linha se o índice está fora de 0..length (sem -O). */
static inline void check_index(int64_t index, int64_t length, int line)
{
    if (ALG_CHECK && (index < 0 || index >= length))
    {
        runtime_error(line, "índice %lld fora de 0..%lld", (long long)index, (long long)length);
    }
}

/* ---------- INF em vetor ': int32' ---------- */

#define ALG_INF 1000000000000000000LL

/* INF (ou mais) vira o maior int32; os outros valores precisam caber abaixo dele */
static inline int64_t encode_int32_inf(int64_t value, int line)
{
    if (value >= ALG_INF)
    {
        return INT32_MAX;
    }

    if (ALG_CHECK && (value < INT32_MIN || value >= INT32_MAX))
    {
        runtime_error(line, "%lld não cabe em int32", (long long)value);
    }

    return value;
}

/** Valor guardado num int32 com INF → valor da linguagem. */
static inline int64_t decode_int32_inf(int32_t stored)
{
    return stored == INT32_MAX ? ALG_INF : stored;
}

/* ---------- aleatório e hash ---------- */

/* semente aleatória por execução: protege o hash e o pivô contra entradas feitas para derrubá-los
 */
static uint64_t hash_seed;
static uint64_t random_state;

/** Mistura os bits (splitmix64): base do hash e do sorteio. */
static inline uint64_t mix_bits(uint64_t bits)
{
    bits += 0x9e3779b97f4a7c15ULL;
    bits = (bits ^ (bits >> 30)) * 0xbf58476d1ce4e5b9ULL;
    bits = (bits ^ (bits >> 27)) * 0x94d049bb133111ebULL;
    return bits ^ (bits >> 31);
}

/** Próximo número aleatório (semente diferente a cada execução). */
static inline uint64_t random_u64(void)
{
    random_state = mix_bits(random_state);
    return random_state;
}

/** Hash de um inteiro com a semente da execução. */
static inline uint64_t hash_int(int64_t value)
{
    return mix_bits((uint64_t)value ^ hash_seed);
}

/** Hash de um real com a semente da execução (-0.0 e 0.0 iguais). */
static inline uint64_t hash_real(double value)
{
    uint64_t raw_bits;

    value = value == 0.0 ? 0.0 : value; /* -0.0 e 0.0 são a mesma chave */
    memcpy(&raw_bits, &value, sizeof raw_bits);
    return mix_bits(raw_bits ^ hash_seed);
}

/* ---------- ordenação por prefixo (chaves de mapa, strings) ---------- */

/* prefixo de 64 bits que preserva a ordem e a posição do item original */
typedef struct
{
    uint64_t prefix;
    uint64_t index;
} OrderKey;

/* desempate de blocos grandes: qsort não leva contexto, então ele fica aqui (um programa, uma thread) */
static int (*order_tie_less)(uint64_t left, uint64_t right, const void *context);
static const void *order_tie_context;

static int order_tie_compare(const void *left, const void *right)
{
    uint64_t a = ((const OrderKey *)left)->index;
    uint64_t b = ((const OrderKey *)right)->index;
    return order_tie_less(a, b, order_tie_context) ? -1 : order_tie_less(b, a, order_tie_context);
}

/** Ordena por prefixo (radix LSD, pula bytes iguais em tudo); empates vão para less (pode ser NULL). */
static void order_sort(OrderKey *keys, int64_t count,
                       int (*less)(uint64_t left, uint64_t right, const void *context),
                       const void *context)
{
    OrderKey *buffer = malloc((size_t)count * sizeof *buffer);
    OrderKey *source = keys;
    uint64_t all_or = 0;
    uint64_t all_and = ~0ULL;

    for (int64_t i = 0; i < count; i++)
    {
        all_or |= keys[i].prefix;
        all_and &= keys[i].prefix;
    }

    for (int shift = 0; shift < 64; shift += 8)
    {
        if ((((all_or ^ all_and) >> shift) & 0xFF) == 0)
        {
            continue;
        }

        int64_t start[256] = {0};

        for (int64_t i = 0; i < count; i++)
        {
            start[(source[i].prefix >> shift) & 0xFF]++;
        }

        for (int64_t digit = 0, total = 0; digit < 256; digit++)
        {
            int64_t size = start[digit];
            start[digit] = total;
            total += size;
        }

        OrderKey *target = source == keys ? buffer : keys;

        for (int64_t i = 0; i < count; i++)
        {
            target[start[(source[i].prefix >> shift) & 0xFF]++] = source[i];
        }

        source = target;
    }

    if (source != keys)
    {
        memcpy(keys, source, (size_t)count * sizeof *keys);
    }

    free(buffer);

    if (!less)
    {
        return;
    }

    /* blocos de prefixo igual: inserção nos pequenos, qsort nos grandes (nunca O(n²)) */
    for (int64_t low = 0; low < count;)
    {
        int64_t high = low + 1;

        while (high < count && keys[high].prefix == keys[low].prefix)
        {
            high++;
        }

        if (high - low > 16)
        {
            order_tie_less = less;
            order_tie_context = context;
            qsort(keys + low, (size_t)(high - low), sizeof *keys, order_tie_compare);
        }
        else
        {
            for (int64_t i = low + 1; i < high; i++)
            {
                OrderKey moving = keys[i];
                int64_t j = i;

                while (j > low && less(moving.index, keys[j - 1].index, context))
                {
                    keys[j] = keys[j - 1];
                    j--;
                }

                keys[j] = moving;
            }
        }

        low = high;
    }
}

/* memória que cresce (estruturas): realloc com checagem */
static void *grow_memory(void *block, size_t bytes)
{
    block = realloc(block, bytes ? bytes : 1);

    if (!block)
    {
        runtime_error(0, "memória esgotada");
    }

    return block;
}

/* ---------- matemática de maratona ---------- */

/** Máximo divisor comum (sempre >= 0). */
static int64_t gcd_int(int64_t left, int64_t right)
{
    uint64_t a = left < 0 ? 0 - (uint64_t)left : (uint64_t)left;
    uint64_t b = right < 0 ? 0 - (uint64_t)right : (uint64_t)right;

    while (b)
    {
        uint64_t rest = a % b;
        a = b;
        b = rest;
    }

    return (int64_t)a;
}

/** Mínimo múltiplo comum (0 se um dos dois é 0). */
static int64_t lcm_int(int64_t left, int64_t right)
{
    if (left == 0 || right == 0)
    {
        return 0;
    }

    int64_t divisor = gcd_int(left, right);
    int64_t value = left / divisor * right;
    return value < 0 ? -value : value;
}

/** base^exponent mod modulus por exponenciação rápida; resultado em [0, modulus). */
static int64_t powmod_int(int64_t base, int64_t exponent, int64_t modulus, int line)
{
    if (modulus <= 0)
    {
        runtime_error(line, "potmod com módulo %lld (precisa ser positivo)", (long long)modulus);
    }

    if (exponent < 0)
    {
        runtime_error(line, "potmod com expoente negativo (%lld)", (long long)exponent);
    }

    unsigned __int128 result = 1 % (uint64_t)modulus;
    unsigned __int128 power = (uint64_t)euclid_mod(base, modulus, line);

    while (exponent > 0)
    {
        if (exponent & 1)
        {
            result = result * power % (uint64_t)modulus;
        }

        power = power * power % (uint64_t)modulus;
        exponent >>= 1;
    }

    return (int64_t)result;
}

/* crivo com roda módulo 30 (como o primesieve): byte b guarda 30b + {1,7,11,13,17,19,23,29},
 * um bit cada; múltiplos de 2, 3 e 5 nem estão no vetor */
enum
{
    SIEVE_SEGMENT_BYTES = 32768,         /* tamanho do L1: cada riscada cai no cache */
    SIEVE_PATTERN_BYTES = 7 * 11 * 13 * 17 /* período do pré-crivo */
};

static const uint8_t SIEVE_RESIDUES[8] = {1, 7, 11, 13, 17, 19, 23, 29};

/* bit do resto r (mod 30); 0xFF para restos que a roda não guarda */
static const uint8_t SIEVE_BIT_OF[30] = {
    0xFF, 0,    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 1,    0xFF, 0xFF, 0xFF, 2,    0xFF, 3,    0xFF,
    0xFF, 0xFF, 4,    0xFF, 5,    0xFF, 0xFF, 0xFF, 6,    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 7};

/* primo que risca: posição do próximo múltiplo (volta t, fase k) e as 8 posições de uma volta */
typedef struct
{
    int64_t cycle;
    int32_t phase;
    int32_t prime;
    uint32_t offsets[8];
    uint8_t masks[8];
} SievingPrime;

/** Padrão periódico com os múltiplos de 7, 11, 13 e 17 já riscados (pré-crivo). */
static uint8_t *sieve_pattern(void)
{
    static uint8_t *pattern;

    if (pattern)
    {
        return pattern;
    }

    pattern = malloc(SIEVE_PATTERN_BYTES);
    memset(pattern, 0xFF, SIEVE_PATTERN_BYTES);
    static const int SMALL_PRIMES[] = {7, 11, 13, 17};

    for (int k = 0; k < 4; k++)
    {
        for (int64_t multiple = SMALL_PRIMES[k]; multiple < 30LL * SIEVE_PATTERN_BYTES;
             multiple += 2 * SMALL_PRIMES[k])
        {
            uint8_t bit = SIEVE_BIT_OF[multiple % 30];

            if (bit != 0xFF)
            {
                pattern[multiple / 30] &= (uint8_t)~(1u << bit);
            }
        }
    }

    return pattern;
}

/** Risca os múltiplos de um primo no segmento [low, low + size) (em bytes). */
static inline void sieve_cross_off(uint8_t *restrict segment, int64_t low, int64_t size,
                                   SievingPrime *restrict sieving)
{
    int64_t prime = sieving->prime;
    int64_t high = low + size;
    int64_t cycle = sieving->cycle;
    int phase = sieving->phase;

    /* até o começo de uma volta */
    while (phase != 0)
    {
        int64_t position = prime * cycle + sieving->offsets[phase];

        if (position >= high)
        {
            sieving->cycle = cycle;
            sieving->phase = phase;
            return;
        }

        segment[position - low] &= sieving->masks[phase];
        phase = (phase + 1) & 7;
        cycle += phase == 0;
    }

    /* voltas inteiras: 8 riscadas com deslocamentos fixos, avança p bytes */
    const uint32_t o0 = sieving->offsets[0], o1 = sieving->offsets[1], o2 = sieving->offsets[2],
                   o3 = sieving->offsets[3], o4 = sieving->offsets[4], o5 = sieving->offsets[5],
                   o6 = sieving->offsets[6], o7 = sieving->offsets[7];
    const uint8_t m0 = sieving->masks[0], m1 = sieving->masks[1], m2 = sieving->masks[2],
                  m3 = sieving->masks[3], m4 = sieving->masks[4], m5 = sieving->masks[5],
                  m6 = sieving->masks[6], m7 = sieving->masks[7];
    int64_t base = prime * cycle - low;

    for (; base + o7 < size; base += prime)
    {
        uint8_t *at = segment + base;
        at[o0] &= m0;
        at[o1] &= m1;
        at[o2] &= m2;
        at[o3] &= m3;
        at[o4] &= m4;
        at[o5] &= m5;
        at[o6] &= m6;
        at[o7] &= m7;
    }

    cycle = (base + low) / prime;

    /* o resto da última volta */
    for (phase = 0;; phase++)
    {
        int64_t position = prime * cycle + sieving->offsets[phase];

        if (position >= high)
        {
            break;
        }

        segment[position - low] &= sieving->masks[phase];
    }

    sieving->cycle = cycle;
    sieving->phase = phase;
}

/** Primos até limit: Eratóstenes segmentado com roda 30, pré-crivo de 7..17 e extração por ctz. */
/* n / d por ponto flutuante (bem mais barato que a divisão de 64 bits), corrigido para ser exato */
static inline int64_t divide_fast(int64_t n, int64_t d, double inverse)
{
    int64_t q = (int64_t)((double)n * inverse);

    while (q * d > n)
    {
        q--;
    }

    while ((q + 1) * d <= n)
    {
        q++;
    }

    return q;
}

/**
 * Quantos primos ≤ n, sem listar (Lucy_Hedgehog, O(n^(3/4)) tempo e O(√n) memória):
 * S(v) = quantos de 2..v sobrevivem ao crivo pelos primos já vistos; só os v = n / i importam.
 */
static int64_t count_primes(int64_t n)
{
    if (n < 2)
    {
        return 0;
    }

    int64_t root = (int64_t)sqrt((double)n);

    while (root * root > n)
    {
        root--;
    }

    while ((root + 1) * (root + 1) <= n)
    {
        root++;
    }

    /* small[v] = S(v) para v ≤ √n; large[i] = S(n / i) para i ≤ √n */
    int64_t *small = malloc((size_t)(root + 2) * sizeof *small);
    int64_t *large = malloc((size_t)(root + 2) * sizeof *large);
    double *inverse = malloc((size_t)(root + 2) * sizeof *inverse);

    if (!small || !large || !inverse)
    {
        runtime_error(0, "memória esgotada");
    }

    for (int64_t v = 0; v <= root; v++)
    {
        small[v] = v - 1;
    }

    small[0] = 0;

    for (int64_t i = 1; i <= root; i++)
    {
        large[i] = n / i - 1;
        inverse[i] = 1.0 / (double)i;
    }

    for (int64_t p = 2; p <= root; p++)
    {
        /* p é primo quando o crivo pelos menores não o tirou */
        if (small[p] == small[p - 1])
        {
            continue;
        }

        int64_t before = small[p - 1];
        int64_t square = p * p;
        int64_t limit = n / square < root ? n / square : root;
        int64_t direct = root / p < limit ? root / p : limit;

        for (int64_t i = 1; i <= direct; i++)
        {
            large[i] -= large[i * p] - before;
        }

        double inverse_p = 1.0 / (double)p;

        for (int64_t i = direct + 1; i <= limit; i++)
        {
            int64_t d = i * p;
            large[i] -= small[divide_fast(n, d, inverse[i] * inverse_p)] - before;
        }

        for (int64_t v = root; v >= square; v--)
        {
            small[v] -= small[divide_fast(v, p, inverse_p)] - before;
        }
    }

    int64_t total = large[1];
    free(small);
    free(large);
    free(inverse);
    return total;
}

static int64_t *sieve_primes(int64_t limit, int64_t *count, int line)
{
    if (limit < 0)
    {
        runtime_error(line, "crivo(%lld): o limite precisa ser >= 0", (long long)limit);
    }

    /* π(x) < 1,25506·x/ln x (Rosser e Schoenfeld): tamanho certo sem crescer o vetor */
    int64_t bound = limit < 100 ? 32 : (int64_t)(1.25506 * (double)limit / log((double)limit)) + 32;
    int64_t *primes = grow_memory(NULL, (size_t)bound * sizeof(int64_t));
    int64_t found = 0;

    for (int64_t small = 2; small <= 5 && small <= limit; small += 1 + (small > 2))
    {
        primes[found++] = small;
    }

    if (limit < 7)
    {
        *count = found;
        return primes;
    }

    int64_t root = (int64_t)sqrt((double)limit);

    while (root * root > limit)
    {
        root--;
    }

    while ((root + 1) * (root + 1) <= limit)
    {
        root++;
    }

    /* primos que riscam: 19..√limit (7..17 já estão no padrão) */
    uint8_t *small_composite = calloc((size_t)root + 1, 1);
    SievingPrime *sieving = malloc(((size_t)root / 2 + 2) * sizeof *sieving);
    int64_t sieving_count = 0;

    for (int64_t p = 3; p <= root; p += 2)
    {
        if (small_composite[p])
        {
            continue;
        }

        for (int64_t multiple = p * p; multiple <= root; multiple += 2 * p)
        {
            small_composite[multiple] = 1;
        }

        if (p < 19)
        {
            continue;
        }

        /* múltiplo p·q com q = 30t + r_k fica no byte p·t + ⌊p·r_k/30⌋ */
        SievingPrime *entry = &sieving[sieving_count++];
        entry->prime = (int32_t)p;

        for (int k = 0; k < 8; k++)
        {
            int64_t product = p * SIEVE_RESIDUES[k];
            entry->offsets[k] = (uint32_t)(product / 30);
            entry->masks[k] = (uint8_t)~(1u << SIEVE_BIT_OF[product % 30]);
        }

        /* começa em p·p: q = p */
        entry->cycle = p / 30;
        entry->phase = SIEVE_BIT_OF[p % 30];
    }

    free(small_composite);

    const uint8_t *pattern = sieve_pattern();
    uint8_t *segment = malloc(SIEVE_SEGMENT_BYTES + 8);
    int64_t last_byte = limit / 30;

    for (int64_t low = 0; low <= last_byte; low += SIEVE_SEGMENT_BYTES)
    {
        int64_t size = last_byte - low + 1 < SIEVE_SEGMENT_BYTES ? last_byte - low + 1
                                                                 : SIEVE_SEGMENT_BYTES;
        int64_t phase = low % SIEVE_PATTERN_BYTES;

        for (int64_t at = 0; at < size;)
        {
            int64_t run = SIEVE_PATTERN_BYTES - phase < size - at ? SIEVE_PATTERN_BYTES - phase
                                                                  : size - at;
            memcpy(segment + at, pattern + phase, (size_t)run);
            at += run;
            phase = 0;
        }

        if (low == 0)
        {
            /* 1 não é primo; 7, 11, 13 e 17 são, mesmo riscados no padrão */
            segment[0] = (uint8_t)((segment[0] & ~1u) | 0x1E);
        }

        for (int64_t k = 0; k < sieving_count; k++)
        {
            sieve_cross_off(segment, low, size, &sieving[k]);
        }

        /* números acima de limit no último byte não contam */
        if (low + size - 1 == last_byte)
        {
            int remainder = (int)(limit % 30);

            for (int bit = 0; bit < 8; bit++)
            {
                if (SIEVE_RESIDUES[bit] > remainder)
                {
                    segment[size - 1] &= (uint8_t)~(1u << bit);
                }
            }
        }

        /* extrai 8 bytes por vez: bit j = byte j/8, resto j%8 */
        memset(segment + size, 0, 8);

        for (int64_t at = 0; at < size; at += 8)
        {
            uint64_t bits;
            memcpy(&bits, segment + at, 8);
            int64_t base = 30 * (low + at);

            while (bits)
            {
                int j = __builtin_ctzll(bits);
                primes[found++] = base + 30 * (j >> 3) + SIEVE_RESIDUES[j & 7];
                bits &= bits - 1;
            }
        }
    }

    free(segment);
    free(sieving);
    *count = found;
    return primes;
}

/* ---------- union-find (conjuntos) ---------- */

typedef struct
{
    int32_t *parent; /* parent[x] == x: x é o representante do grupo */
    int32_t *size;   /* tamanho do grupo (válido no representante) */
    int64_t count;   /* número de elementos */
    int64_t groups;  /* número de grupos */
} DisjointSets;

/** n elementos, cada um no seu próprio grupo. */
static DisjointSets *dsu_new(int64_t count, int line)
{
    if (count < 0 || count > INT32_MAX)
    {
        runtime_error(line, "conjuntos(%lld): tamanho inválido", (long long)count);
    }

    DisjointSets *sets = arena_allocate(sizeof(DisjointSets));
    sets->parent = arena_allocate((size_t)count * sizeof(int32_t));
    sets->size = arena_allocate((size_t)count * sizeof(int32_t));
    sets->count = count;
    sets->groups = count;

    for (int64_t i = 0; i < count; i++)
    {
        sets->parent[i] = (int32_t)i;
        sets->size[i] = 1;
    }

    return sets;
}

/** Representante do grupo de x (path halving: o caminho encurta a cada busca). */
static int64_t dsu_find(DisjointSets *sets, int64_t element, int line)
{
    check_index(element, sets->count, line);

    int32_t x = (int32_t)element;

    while (sets->parent[x] != x)
    {
        sets->parent[x] = sets->parent[sets->parent[x]];
        x = sets->parent[x];
    }

    return x;
}

/** Junta os grupos de a e b (o menor entra no maior); 1 se eram grupos diferentes. */
static int64_t dsu_merge(DisjointSets *sets, int64_t a, int64_t b, int line)
{
    int32_t root_a = (int32_t)dsu_find(sets, a, line);
    int32_t root_b = (int32_t)dsu_find(sets, b, line);

    if (root_a == root_b)
    {
        return 0;
    }

    if (sets->size[root_a] < sets->size[root_b])
    {
        int32_t swap = root_a;
        root_a = root_b;
        root_b = swap;
    }

    sets->parent[root_b] = root_a;
    sets->size[root_a] += sets->size[root_b];
    sets->groups--;
    return 1;
}

/** 1 se a e b estão no mesmo grupo. */
static int64_t dsu_same(DisjointSets *sets, int64_t a, int64_t b, int line)
{
    return dsu_find(sets, a, line) == dsu_find(sets, b, line);
}

/** Tamanho do grupo de x. */
static int64_t dsu_group_size(DisjointSets *sets, int64_t element, int line)
{
    return sets->size[dsu_find(sets, element, line)];
}

/* ---------- início: roda o programa numa thread com pilha de 256 MB ---------- */

static void *program_thread(void *unused)
{
    (void)unused;
    alg_main();
    return NULL;
}

/** Compila o .alg; com -o gera o executável, senão compila e roda. */
int main(void)
{
    pthread_attr_t thread_attributes;
    pthread_t thread;

    hash_seed = mix_bits((uint64_t)time(NULL) ^ (uint64_t)(uintptr_t)&thread_attributes);
    random_state = hash_seed;

    pthread_attr_init(&thread_attributes);
    pthread_attr_setstacksize(&thread_attributes, (size_t)256 << 20);

    if (pthread_create(&thread, &thread_attributes, program_thread, NULL) != 0)
    {
        alg_main();
    }
    else
    {
        pthread_join(thread, NULL);
    }

    flush_output();
    return 0;
}
