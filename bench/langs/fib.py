import sys
def fib(k):
    return k if k < 2 else fib(k - 1) + fib(k - 2)
print(fib(int(sys.stdin.read())))
