use std::io::*;

fn main() {
    let mut s = String::new();
    stdin().read_to_string(&mut s).unwrap();
    let mut it = s.split_ascii_whitespace().map(|x| x.parse::<i64>().unwrap());
    let n = it.next().unwrap() as usize;
    let mut v: Vec<i64> = it.take(n).collect();
    v.sort_unstable();
    let mut total: i64 = 0;
    for (i, x) in v.iter().enumerate() {
        total = (total + x % 1000003 * (i as i64 % 1000)) % 1000000007;
    }
    println!("{}", total);
}
