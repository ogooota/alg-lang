#lang en
#decl seen: set {str}
s = set {5, 1, 3}
s.add(2)
s.add(5)
s.remove(1)
write(s.size(), 3 in s, 1 in s)
for x in s write(x)
t = set {3, 4}
u = s union t
i = s inter t
d = s minus t
for x in u write("u", x)
for x in i write("i", x)
for x in d write("d", x)
seen.add("bob")
seen.add("ana")
seen.add("bob")
for w in seen write(w)
ps = set {}
ps.add((2, 1))
ps.add((1, 9))
ps.add((2, 1))
for p in ps write(p)
fn count_in(a: set {int}) -> int {
    ret a.size()
}
write(count_in(u))
