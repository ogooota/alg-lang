CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -g
SRC = src/util.c src/language.c src/lex.c src/parse.c src/sema.c src/sema_types.c src/sema_scope.c src/sema_expr.c \
      src/range.c src/gen.c src/gen_types.c src/gen_lazy.c src/gen_expr.c src/gen_stmt.c src/main.c

alg: $(SRC) src/alg.h src/sema_internal.h src/gen_internal.h build/rt_core.inc build/rt_arr.inc build/rt_heap.inc build/rt_queue.inc build/rt_map.inc build/rt_graph.inc build/rt_fenwick.inc build/rt_segtree.inc build/rt_rmq.inc
	$(CC) $(CFLAGS) -D_GNU_SOURCE -Ibuild -o $@ $(SRC)

build/rt_%.inc: rt/%.h
	@mkdir -p build
	xxd -i $< > $@

test: alg
	./tests/run.sh

format:
	clang-format -i src/*.c src/*.h rt/*.h

clean:
	rm -rf alg build

.PHONY: test format clean
