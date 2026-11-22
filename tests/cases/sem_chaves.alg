#lang en
// a single-statement body needs no braces, like in C
for i=0 to 8
    if i > 5
        write("hello", i)

total = 0
for i=0 to 10
    if i % 2 == 0
        total += i
    else
        total -= 1
write(total)

fn sign(x)
    if x < 0
        ret -1
    else if x == 0
        ret 0
    else
        ret 1
write(sign(-5), sign(0), sign(7))

// "else" binds to the nearest if, as in C
x = 3
if x > 0
    if x > 10
        write("big")
    else
        write("small positive")

i = 0
while i < 3
    inc i
write(i)
for k=0 to 3 write(k)
