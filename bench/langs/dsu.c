#include "io.h"

static int32_t *parent;
static int32_t *size;

static int32_t find(int32_t x)
{
    while (parent[x] != x)
    {
        parent[x] = parent[parent[x]];
        x = parent[x];
    }

    return x;
}

int main(void)
{
    int64_t n = read_int();
    int64_t q = read_int();
    int64_t groups = n;
    int64_t same = 0;
    parent = malloc(n * sizeof *parent);
    size = malloc(n * sizeof *size);

    for (int64_t i = 0; i < n; i++)
    {
        parent[i] = (int32_t)i;
        size[i] = 1;
    }

    for (int64_t k = 0; k < q; k++)
    {
        int64_t type = read_int();
        int32_t a = find((int32_t)read_int());
        int32_t b = find((int32_t)read_int());

        if (type == 1)
        {
            if (a != b)
            {
                if (size[a] < size[b])
                {
                    int32_t swap = a;
                    a = b;
                    b = swap;
                }

                parent[b] = a;
                size[a] += size[b];
                groups--;
            }
        }
        else if (a == b)
        {
            same++;
        }
    }

    write_int(same, ' ');
    write_int(groups, '\n');
    flush_output();
}
