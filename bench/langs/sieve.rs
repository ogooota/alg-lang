use std::io::*;

fn main() {
    let mut s = String::new();
    stdin().read_to_string(&mut s).unwrap();
    let n: usize = s.trim().parse().unwrap();
    let mut composite = vec![false; n + 1];
    let mut total = 0u64;
    for i in 2..=n {
        if !composite[i] {
            total += 1;
            let mut j = i * i;
            while j <= n {
                composite[j] = true;
                j += i;
            }
        }
    }
    println!("{}", total);
}
