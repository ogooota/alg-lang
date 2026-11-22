#include "io.h"

int main(void)
{
    int64_t n = read_int();
    char *composite = calloc(n + 1, 1);
    int64_t total = 0;

    for (int64_t i = 2; i <= n; i++)
    {
        if (!composite[i])
        {
            total++;

            for (int64_t j = i * i; j <= n; j += i)
            {
                composite[j] = 1;
            }
        }
    }

    write_int(total, '\n');
    flush_output();
}
