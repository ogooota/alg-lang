#lang en
fn total(v: {int}, k: int) -> int {
    s = 0
    for x in v s += x * k
    ret s
}
fn half(x: real) -> real {
    ret x / 2
}
fn fib(n: int) -> int {
    if (n < 2) ret n
    ret fib(n - 1) + fib(n - 2)
}
fn best(items: {(int, str)}) -> (int, str) {
    b = items[0]
    for p in items
        if (p > b) b = p
    ret b
}
fn hello(name) {
    write("hi", name)
}
write(total({1, 2, 3}, 2), half(3), fib(20))
write(best({(1, "a"), (3, "c"), (2, "b")}))
hello("x")
e = {}
write(total(e, 1))
