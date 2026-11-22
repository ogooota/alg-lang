#!/bin/sh
# roda cada tests/cases/X.alg com X.in (se existir) e compara com X.out
cd "$(dirname "$0")/.." || exit 1
fail=0
for src in tests/cases/*.alg; do
    base=${src%.alg}
    input=/dev/null
    [ -f "$base.in" ] && input=$base.in
    if ./alg "$src" < "$input" > "$base.got" 2>&1 && cmp -s "$base.got" "$base.out"; then
        echo "ok    $src"
        rm -f "$base.got"
    else
        echo "FALHA $src (veja diff $base.out $base.got)"
        fail=1
    fi
done
exit $fail
