use std::io::*;

fn fib(k: i64) -> i64 {
    if k < 2 { k } else { fib(k - 1) + fib(k - 2) }
}

fn main() {
    let mut s = String::new();
    stdin().read_to_string(&mut s).unwrap();
    println!("{}", fib(s.trim().parse().unwrap()));
}
