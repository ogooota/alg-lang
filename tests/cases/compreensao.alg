#lang en
v = {5, -2, 8, -1, 3}
pos = {x for x in v if x > 0}
sq = {i * i for i=0 to 5}
write(pos, sq)
names = {"ana", "bo", "carl"}
long = {s + "!" for s in names if s.size() > 2}
write(long)
mods = set {x % 3 for x in v}
for m in mods write("m", m)
write(sum(x for x in v if x > 0), max(x for x in v if x < 0), sum(i for i=1 thr 10 if i % 2 == 0))
write(all(x != 0 for x in v), exists(x > 7 for x in v), some(x > 100 for x in v), all(x > 0 for x in v))
s = set {1, 2, 3}
write(sum(x for x in s), exists(x == 2 for x in s))
pairs = {(i, i * 2) for i=0 to 3}
write(pairs)
write(pairs.size())
if (all(x * x >= 0 for x in v)) write("ok")
