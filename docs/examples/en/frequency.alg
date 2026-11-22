#lang en
read(n)
count = map {}
for i=0 to n {
    read(x)
    count[x] += 1
}
for (value, times) in count {
    write(value, times)
}
