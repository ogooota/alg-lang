use std::io::*;

fn main() {
    let mut s = String::new();
    stdin().read_to_string(&mut s).unwrap();
    let mut it = s.split_ascii_whitespace().map(|x| x.parse::<i64>().unwrap());
    let n = it.next().unwrap() as usize;
    let cap = it.next().unwrap() as usize;
    let w: Vec<usize> = (0..n).map(|_| it.next().unwrap() as usize).collect();
    let v: Vec<i64> = (0..n).map(|_| it.next().unwrap()).collect();
    let mut dp = vec![0i64; cap + 1];
    for i in 0..n {
        for c in (w[i]..=cap).rev() {
            dp[c] = dp[c].max(dp[c - w[i]] + v[i]);
        }
    }
    println!("{}", dp[cap]);
}
