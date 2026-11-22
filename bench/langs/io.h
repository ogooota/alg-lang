/* Leitura com fread e saída bufferizada: o jeito usual de C em maratona. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char input[1 << 16];
static size_t input_length;
static size_t input_position;
static char output[1 << 16];
static size_t output_length;

static int next_byte(void)
{
    if (input_position == input_length)
    {
        input_length = fread(input, 1, sizeof input, stdin);
        input_position = 0;

        if (!input_length)
        {
            return -1;
        }
    }

    return input[input_position++];
}

static int64_t read_int(void)
{
    int c = next_byte();

    while (c == ' ' || c == '\n' || c == '\r')
    {
        c = next_byte();
    }

    int negative = c == '-';

    if (negative)
    {
        c = next_byte();
    }

    int64_t value = 0;

    while (c >= '0' && c <= '9')
    {
        value = value * 10 + (c - '0');
        c = next_byte();
    }

    return negative ? -value : value;
}

/** Lê uma palavra em word (até 63 bytes) e devolve o tamanho. */
static int read_word(char *word)
{
    int c = next_byte();
    int length = 0;

    while (c == ' ' || c == '\n' || c == '\r')
    {
        c = next_byte();
    }

    while (c > ' ')
    {
        word[length++] = (char)c;
        c = next_byte();
    }

    word[length] = 0;
    return length;
}

static void flush_output(void)
{
    fwrite(output, 1, output_length, stdout);
    output_length = 0;
}

static void write_bytes(const char *text, size_t length)
{
    if (output_length + length > sizeof output)
    {
        flush_output();
    }

    memcpy(output + output_length, text, length);
    output_length += length;
}

static void write_int(int64_t value, char end)
{
    char digits[24];
    int length = snprintf(digits, sizeof digits, "%lld%c", (long long)value, end);
    write_bytes(digits, (size_t)length);
}
