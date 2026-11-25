#lang en
#decl name: str
read(n)
people = {}
for i=0 to n {
    read(name, score)
    people.add((-score, name))
}
sort(people)
for [s, who] in people
    write(who, -s)
[s, who] = people[0]
who[0] = 'X'
write(who, people[0])
p = ("ab", 1)
q = ("ab", 2)
write(p < q, p == ("ab", 1), ("b", 0) < ("ab", 9))
m = map {}
m[("x", 1)] += 5
m[("x", 1)] += 2
m[("y", 0)] = 1
for [k, v] in m
    write(k, v)
h = heap()
h.push(("zed", 1))
h.push(("amy", 2))
write(h.top())
vs = {}
vs.add("ab")
s2 = vs[0]
s2[0] = 'X'
write(s2, vs[0])
mm = map {}
mm[1] = "cd"
t2 = mm[1]
t2[0] = 'Y'
write(t2, mm[1])
for x in vs {
    x[1] = 'Z'
}
write(vs)
