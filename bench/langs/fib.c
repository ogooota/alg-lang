#include "io.h"

static int64_t fib(int64_t k)
{
    return k < 2 ? k : fib(k - 1) + fib(k - 2);
}

int main(void)
{
    write_int(fib(read_int()), '\n');
    flush_output();
}
