use std::io::*;

fn find(parent: &mut [u32], mut x: usize) -> usize {
    while parent[x] as usize != x {
        parent[x] = parent[parent[x] as usize];
        x = parent[x] as usize;
    }
    x
}

fn main() {
    let mut s = String::new();
    stdin().read_to_string(&mut s).unwrap();
    let mut it = s.split_ascii_whitespace().map(|x| x.parse::<usize>().unwrap());
    let n = it.next().unwrap();
    let q = it.next().unwrap();
    let mut parent: Vec<u32> = (0..n as u32).collect();
    let mut size = vec![1u32; n];
    let (mut groups, mut same) = (n, 0u64);
    for _ in 0..q {
        let t = it.next().unwrap();
        let mut a = find(&mut parent, it.next().unwrap());
        let mut b = find(&mut parent, it.next().unwrap());
        if t == 1 {
            if a != b {
                if size[a] < size[b] {
                    std::mem::swap(&mut a, &mut b);
                }
                parent[b] = a as u32;
                size[a] += size[b];
                groups -= 1;
            }
        } else if a == b {
            same += 1;
        }
    }
    println!("{} {}", same, groups);
}
