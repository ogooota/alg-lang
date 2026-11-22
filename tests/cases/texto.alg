#lang en
#decl {
    word: str;
    names: str;
}
// palavras, caracteres, contas com caractere
read(word)
count = {}
for i=0 to 26
    count.add(0)
for c in word
    count[c - 'a'] += 1
write(word, len(word), count[0], count[1])
write(word[0], word[len(word) - 1], word[1..3])
if word[0] == 'b'
    write("starts with b")

// mutável, add, +, comparação, str()
s = "abc"
s[0] = 'x'
s.add('d')
s.add("ef")
t = s + "!" + 'z'
write(s, t, len(t), s < t, "abc" < "abd", "ab" < "abc", s == "xbcdef")
write(str(42) + "/" + str(-7), str(2.5))
answer = ""
for i=0 to 5
    answer.add('a' + i)
write(answer)

// vetor de strings: ler, ordenar, guardar cópia
read(n, names[0..n])
sort(names)
write(names)
saved = {}
saved.add(word)
word[0] = 'Z'
write(saved[0], word)

// mapa com chave string, em ordem de dicionário
freq = map {}
for w in names
    freq[w] += 1
for (w, k) in freq
    write(w, k)
write("carol" in freq, "zed" in freq)

// linha inteira logo depois de um número
read(m)
read_line(line)
write(line)
read_line(other)
write(other, len(other))
