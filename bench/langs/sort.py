import sys
data = sys.stdin.buffer.read().split()
n = int(data[0])
v = sorted(map(int, data[1:n + 1]))
total = 0
for i, x in enumerate(v):
    total = (total + x % 1000003 * (i % 1000)) % 1000000007
print(total)
