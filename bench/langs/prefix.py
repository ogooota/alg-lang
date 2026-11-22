import sys
from itertools import accumulate
data = sys.stdin.buffer.read().split()
n = int(data[0])
prefix = [0] + list(accumulate(map(int, data[1:n + 1])))
q = int(data[n + 1])
pos = n + 2
out = []
for k in range(q):
    out.append(prefix[int(data[pos + 1])] - prefix[int(data[pos])])
    pos += 2
sys.stdout.write("\n".join(map(str, out)) + "\n")
