#lang en
#decl {
    item: {(int, int)};
    names: map {str: int};
    grid: {{int}};
    best: (int, str);
    words: {str};
    total: real;
}
read(n, item[0..n])
write(item, item[1][0])
names["ana"] += 2
names["bo"] = 7
for [k, v] in names
    write(k, v)
read(r, c, grid[0..r][0..c])
write(grid[1][1])
read(best)
write(best, best[1])
words.add("x")
words.add("yz")
write(words, words.size())
total = 1
write(total / 2)
