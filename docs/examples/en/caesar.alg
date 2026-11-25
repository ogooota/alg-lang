#lang en
#decl line: str
read(k)
read_line(line)
answer = ""
for c in line {
    if (c >= 'a' && c <= 'z')
        c = 'a' + (c - 'a' + k) % 26
    answer.add(c)
}
write(answer)
