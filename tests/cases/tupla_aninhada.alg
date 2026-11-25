#lang en
#decl e: {(int, (int, str))}
p = (1, (2, "ab"))
write(p, p[1][1], p[1][0])
p[1][0] = 9
[a, b] = p
[c, d] = b
write(a, c, d)
v = {(3, (1, "z")), (3, (0, "y")), (1, (5, "x"))}
sort(v)
write(v)
write((1, (2, 3)) < (1, (2, 4)), (1, (2, 3)) == (1, (2, 3)))
m = map {}
m[(1, (2, 3))] += 4
write(m[(1, (2, 3))], m.size())
h = heap()
h.push(((2, 1), 5))
h.push(((1, 9), 7))
write(h.top())
read(n, e[0..n])
write(e)
fn first(q: ((int, int), int)) -> (int, int) {
    ret q[0]
}
write(first(((4, 5), 6)))
