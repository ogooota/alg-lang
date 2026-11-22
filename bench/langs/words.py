import sys
from collections import Counter
data = sys.stdin.buffer.read().split()
n = int(data[0])
count = Counter(data[1:n + 1])
sys.stdout.buffer.write(b"".join(w + b" " + str(k).encode() + b"\n" for w, k in sorted(count.items())))
