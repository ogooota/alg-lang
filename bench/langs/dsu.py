import sys
data = sys.stdin.buffer.read().split()
n, q = int(data[0]), int(data[1])
parent = list(range(n))
size = [1] * n
groups, same = n, 0
def find(x):
    while parent[x] != x:
        parent[x] = parent[parent[x]]
        x = parent[x]
    return x
pos = 2
for _ in range(q):
    t = data[pos]
    a, b = find(int(data[pos + 1])), find(int(data[pos + 2]))
    pos += 3
    if t == b"1":
        if a != b:
            if size[a] < size[b]:
                a, b = b, a
            parent[b] = a
            size[a] += size[b]
            groups -= 1
    elif a == b:
        same += 1
print(same, groups)
