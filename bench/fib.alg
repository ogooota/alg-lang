#lang en
fn fib(k) {
    if k < 2 {
        ret k
    }
    ret fib(k - 1) + fib(k - 2)
}

read(n)
write(fib(n))
