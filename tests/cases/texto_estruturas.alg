#lang en
names = {"carol", "al", "bob", "alexandre"}
write(names, names.size(), min(names), max(names))
s = names[0]
s.add('!')
write(s, names[0])
names[1] = s
s[0] = 'X'
write(names[1], s)
last = names.pop()
write(last, names.size())
h = heap()
for w in names
    h.push(w)
h.push("aaa")
while h.size() > 0
    write(h.pop())
q = queue()
q.push("first")
q.push("second")
write(q.front(), q.pop(), q.pop())
nick = map {}
nick["bob"] = "bobby"
nick["al"] = "alan" + "!"
for [k, v] in nick
    write(k, v, v.size())
write(nick["bob"], "al" in nick, "zz" in nick)
nick.remove("al")
write("al" in nick, nick.size())
sort(names)
write(names)
