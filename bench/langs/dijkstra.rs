use std::cmp::Reverse;
use std::collections::BinaryHeap;
use std::io::*;

fn main() {
    let mut s = String::new();
    stdin().read_to_string(&mut s).unwrap();
    let mut it = s.split_ascii_whitespace().map(|x| x.parse::<i64>().unwrap());
    let n = it.next().unwrap() as usize;
    let m = it.next().unwrap() as usize;
    let mut graph: Vec<Vec<(usize, i64)>> = vec![Vec::new(); n];
    for _ in 0..m {
        let u = it.next().unwrap() as usize;
        let v = it.next().unwrap() as usize;
        let w = it.next().unwrap();
        graph[u].push((v, w));
    }
    let mut dist = vec![i64::MAX; n];
    let mut heap = BinaryHeap::new();
    dist[0] = 0;
    heap.push(Reverse((0i64, 0usize)));
    while let Some(Reverse((d, u))) = heap.pop() {
        if d > dist[u] {
            continue;
        }
        for &(v, w) in &graph[u] {
            if d + w < dist[v] {
                dist[v] = d + w;
                heap.push(Reverse((dist[v], v)));
            }
        }
    }
    let total: i64 = dist.iter().filter(|&&d| d != i64::MAX).sum();
    println!("{}", total);
}
