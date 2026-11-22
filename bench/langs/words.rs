use std::collections::HashMap;
use std::io::*;

fn main() {
    let mut s = String::new();
    stdin().read_to_string(&mut s).unwrap();
    let mut it = s.split_ascii_whitespace();
    let n: usize = it.next().unwrap().parse().unwrap();
    let mut count: HashMap<&str, i64> = HashMap::new();
    for word in it.take(n) {
        *count.entry(word).or_insert(0) += 1;
    }
    let mut items: Vec<(&str, i64)> = count.into_iter().collect();
    items.sort_unstable();
    let mut out = String::with_capacity(16 << 20);
    for (word, k) in items {
        out.push_str(word);
        out.push(' ');
        out.push_str(&k.to_string());
        out.push('\n');
    }
    stdout().write_all(out.as_bytes()).unwrap();
}
