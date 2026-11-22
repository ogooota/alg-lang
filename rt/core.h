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

__attribute__((noreturn, format(printf, 2, 3))) static void runtime_error(int line,
                                                                          const char *format, ...)
{
    va_list arguments;

    flush_output();
    fprintf(stderr, "erro (linha %d): ", line);
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

/** Primos até limit (crivo de Eratóstenes); devolve o vetor e a quantidade em count. */
static int64_t *sieve_primes(int64_t limit, int64_t *count, int line)
{
    if (limit < 0)
    {
        runtime_error(line, "crivo(%lld): o limite precisa ser >= 0", (long long)limit);
    }

    uint8_t *composite = calloc((size_t)limit + 2, 1);
    int64_t *primes = grow_memory(NULL, (size_t)(limit / 2 + 2) * sizeof(int64_t));
    int64_t found = 0;

    for (int64_t i = 2; i <= limit; i++)
    {
        if (composite[i])
        {
            continue;
        }

        primes[found] = i;
        found++;

        for (int64_t multiple = i * i; multiple <= limit; multiple += i)
        {
            composite[multiple] = 1;
        }
    }

    free(composite);
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
