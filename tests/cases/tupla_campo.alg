#lang en
p = (3, "ana", 2.5)
write(p[0], p[1], p[2])
p[0] += 10
p[1] = "bia"
p[2] = p[2] * 2
write(p)
item = {(1, 2), (3, 4)}
for i=0 to item.size() {
    item[i][0] += 1
}
write(item, item[1][1])
m = map {}
m[5] = (7, 8)
m[5][1] = 9
write(m[5][0], m[5][1], m.size(), m[6][0], m.size())
h = heap()
h.push((2, 1))
write(h.top()[0])
name = p[1]
name[0] = 'Z'
write(name, p[1])
