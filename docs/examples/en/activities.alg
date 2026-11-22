#lang en
read(n)
tasks = {}
for i=0 to n {
    read(start, end)
    tasks.add((end, start))
}
sort(tasks)
count = 0
free_at = 0
for (end, start) in tasks {
    if start >= free_at {
        inc count
        free_at = end
    }
}
write(count)
