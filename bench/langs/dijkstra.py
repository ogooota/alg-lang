import sys
from heapq import heappush, heappop
data = sys.stdin.buffer.read().split()
n, m = int(data[0]), int(data[1])
graph = [[] for _ in range(n)]
pos = 2
for _ in range(m):
    graph[int(data[pos])].append((int(data[pos + 1]), int(data[pos + 2])))
    pos += 3
INF = float("inf")
dist = [INF] * n
dist[0] = 0
heap = [(0, 0)]
while heap:
    d, u = heappop(heap)
    if d > dist[u]:
        continue
    for v, w in graph[u]:
        if d + w < dist[v]:
            dist[v] = d + w
            heappush(heap, (dist[v], v))
print(sum(d for d in dist if d != INF))
