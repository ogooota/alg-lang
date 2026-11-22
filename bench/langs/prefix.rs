use std::io::*;
use std::fmt::Write as _;

fn main() {
    let mut s = String::new();
    stdin().read_to_string(&mut s).unwrap();
    let mut it = s.split_ascii_whitespace().map(|x| x.parse::<i64>().unwrap());
    let n = it.next().unwrap() as usize;
    let mut prefix = vec![0i64; n + 1];
    for i in 0..n {
        prefix[i + 1] = prefix[i] + it.next().unwrap();
    }
    let q = it.next().unwrap();
    let mut out = String::with_capacity(16 << 20);
    for _ in 0..q {
        let l = it.next().unwrap() as usize;
        let r = it.next().unwrap() as usize;
        writeln!(out, "{}", prefix[r] - prefix[l]).unwrap();
    }
    stdout().write_all(out.as_bytes()).unwrap();
}
