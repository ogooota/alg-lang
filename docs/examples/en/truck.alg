#lang en
fn fits(weights, cap, k) {
    trips = 1
    load = 0
    for w in weights {
        if w > cap {
            ret 0
        }
        if load + w > cap {
            inc trips
            load = 0
        }
        load += w
    }
    ret trips <= k
}

read(n, k, weights[0..n])
lo = 1
hi = sum(weights)
while lo < hi {
    mid = (lo + hi) / 2
    if fits(weights, mid, k) {
        hi = mid
    } else {
        lo = mid + 1
    }
}
write(lo)
