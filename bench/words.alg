#lang en
// frequency of each word, printed in dictionary order
#decl word: str
read(n)
count = map {}
for i=0 to n {
    read(word)
    count[word] += 1
}
for [w, k] in count
    write(w, k)
