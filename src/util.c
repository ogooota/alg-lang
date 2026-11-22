/* Utilidades do compilador: alocação, buffer de texto e mensagens de erro. */
#include "alg.h"

const char *source_file_name = "?";

/* ---------- utilidades ---------- */

void *allocate_zeroed(size_t size)
{
    void *memory = calloc(1, size ? size : 1);

    if (!memory)
    {
        perror("alg");
        exit(1);
    }

    return memory;
}

/** Cópia de um texto em memória nova (nunca devolve NULL). */
char *duplicate_string(const char *text)
{
    char *copy = allocate_zeroed(strlen(text) + 1);

    strcpy(copy, text);
    return copy;
}

/** Acrescenta um texto ao fim do buffer, crescendo-o se preciso. */
void text_append(TextBuffer *out, const char *text)
{
    size_t length = strlen(text);

    if (out->count + length + 1 > out->capacity)
    {
        out->capacity = (out->count + length + 1) * 2;
        out->text = realloc(out->text, out->capacity);
    }

    memcpy(out->text + out->count, text, length + 1);
    out->count += length;
}

/** Acrescenta texto formatado (como printf) ao fim do buffer. */
void text_appendf(TextBuffer *out, const char *format, ...)
{
    char temporary[4096];
    va_list arguments;

    va_start(arguments, format);
    int count = vsnprintf(temporary, sizeof temporary, format, arguments);
    va_end(arguments);

    if (count < (int)sizeof temporary)
    {
        text_append(out, temporary);
        return;
    }

    char *large = allocate_zeroed((size_t)count + 1);

    va_start(arguments, format);
    vsnprintf(large, (size_t)count + 1, format, arguments);
    va_end(arguments);

    text_append(out, large);
    free(large);
}

/** Mostra "arquivo:linha:coluna: erro: ..." e encerra o compilador. */
_Noreturn void fail_at(int line, int column, const char *format, ...)
{
    va_list arguments;

    va_start(arguments, format);
    fprintf(stderr, "%s:%d:%d: erro: ", source_file_name, line, column);
    vfprintf(stderr, format, arguments);
    fputc('\n', stderr);
    va_end(arguments);
    exit(1);
}
