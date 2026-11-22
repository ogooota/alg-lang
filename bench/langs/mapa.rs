use std::collections::HashMap;
use std::io::*;

fn main() {
    let mut s = String::new();
    stdin().read_to_string(&mut s).unwrap();
    let mut it = s.split_ascii_whitespace().map(|x| x.parse::<i64>().unwrap());
    let n = it.next().unwrap();
    let mut count: HashMap<i64, i64> = HashMap::new();
    for _ in 0..n {
        *count.entry(it.next().unwrap()).or_insert(0) += 1;
    }
    let best = count.values().max().copied().unwrap_or(0);
    println!("{} {}", count.len(), best);
}
