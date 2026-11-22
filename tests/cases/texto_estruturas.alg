#lang en
names = {"carol", "al", "bob", "alexandre"}
write(names, len(names), min(names), max(names))
s = names[0]
s.add('!')
write(s, names[0])
names[1] = s
s[0] = 'X'
write(names[1], s)
last = names.pop()
write(last, len(names))
h = heap()
for w in names
    h.push(w)
h.push("aaa")
while len(h) > 0
    write(h.pop())
q = queue()
q.push("first")
q.push("second")
write(q.front(), q.pop(), q.pop())
nick = map {}
nick["bob"] = "bobby"
nick["al"] = "alan" + "!"
for (k, v) in nick
    write(k, v, len(v))
write(nick["bob"], "al" in nick, "zz" in nick)
nick.remove("al")
write("al" in nick, len(nick))
sort(names)
write(names)
