import sys
n = int(sys.stdin.read())
composite = bytearray(n + 1)
total = 0
for i in range(2, n + 1):
    if not composite[i]:
        total += 1
        if i * i <= n:
            composite[i * i::i] = b"\x01" * len(range(i * i, n + 1, i))
print(total)
