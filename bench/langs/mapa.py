import sys
from collections import Counter
data = sys.stdin.buffer.read().split()
n = int(data[0])
count = Counter(data[1:n + 1])
print(len(count), max(count.values()))
