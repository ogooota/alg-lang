#include "io.h"

/* hash de palavras com endereçamento aberto; depois ordena as distintas com qsort */
typedef struct
{
    char text[16];
    int64_t count;
} Word;

static int compare(const void *left, const void *right)
{
    return strcmp(((const Word *)left)->text, ((const Word *)right)->text);
}

int main(void)
{
    int64_t n = read_int();
    size_t capacity = 1 << 21;
    Word *table = calloc(capacity, sizeof *table);
    int64_t distinct = 0;
    char word[64];

    for (int64_t i = 0; i < n; i++)
    {
        int length = read_word(word);
        uint64_t hash = 1469598103934665603ULL;

        for (int k = 0; k < length; k++)
        {
            hash = (hash ^ (unsigned char)word[k]) * 1099511628211ULL;
        }

        size_t slot = hash & (capacity - 1);

        while (table[slot].count && strcmp(table[slot].text, word))
        {
            slot = (slot + 1) & (capacity - 1);
        }

        if (!table[slot].count)
        {
            memcpy(table[slot].text, word, length + 1);
            distinct++;
        }

        table[slot].count++;
    }

    Word *items = malloc(distinct * sizeof *items);
    int64_t used = 0;

    for (size_t slot = 0; slot < capacity; slot++)
    {
        if (table[slot].count)
        {
            items[used++] = table[slot];
        }
    }

    qsort(items, distinct, sizeof *items, compare);

    for (int64_t i = 0; i < distinct; i++)
    {
        write_bytes(items[i].text, strlen(items[i].text));
        write_bytes(" ", 1);
        write_int(items[i].count, '\n');
    }

    flush_output();
}
