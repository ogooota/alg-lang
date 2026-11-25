" destaque de sintaxe da linguagem alg (palavras em pt e en)
if exists("b:current_syntax")
    finish
endif

" diretivas
syntax match algDirective "^\s*#\(lang\|decl\)\>"
syntax match algLang "\(#lang\s\+\)\@<=\(pt\|en\)\>"

" controle de fluxo
syntax keyword algConditional se senao if else
syntax keyword algRepeat enquanto para ate em thr while for to in
syntax keyword algStatement ret pare continue break
syntax keyword algKeyword fn nextgroup=algFuncName skipwhite
syntax match algFuncName "\h\w*" contained

" função de uma linha: f(x) = ...
syntax match algFuncName "^\h\w*\ze(.*)\s*=[^=]"

" lógica em palavras
syntax keyword algOperatorWord e ou nao and or not

" valores
syntax keyword algBoolean verdadeiro falso true false
syntax keyword algConstant INF
syntax match algNumber "\<\d\+\>"
syntax match algFloat "\<\d\+\.\d\+\>"

" tipos
syntax keyword algType int real int32

" nomes nativos
syntax keyword algBuiltin ler escrever copia soma min max ordenar heap fila grafo mapa conj prefixo busca mdc mmc potmod crivo conta_primos conjuntos fenwick segtree rmq texto ler_linha
syntax keyword algBuiltin read write copy sum sort queue graph map set prefix search gcd lcm powmod sieve count_primes dsu str read_line
syntax keyword algWord maior dirigido greater directed

" métodos das estruturas (depois do ponto)
syntax match algMethod "\.\zs\(adicionar\|remover_ultimo\|inserir\|remover\|topo\|enfileirar\|desenfileirar\|frente\|aresta\|vizinhos\)\>"
syntax match algMethod "\.\zs\(add\|pop\|push\|top\|front\|edge\|neighbors\|remove\)\>"
syntax match algMethod "\.\zs\(unir\|achar\|juntos\|grupos\|somar\|consulta\|mudar\|merge\|find\|same\|groups\|size\|query\|update\|sum\|soma\|tam\)\>"

" reservadas para os próximos marcos
syntax keyword algReserved todo existe algum uniao inter menos tipo all exists some union minus type

" operadores e símbolos matemáticos opcionais
syntax match algOperator "\.\.\|==\|!=\|<=\|>=\|<<\|>>\|[-+*/%&|^~<>=]"
syntax match algOperator "≤\|≥\|≠\|←\|∈\|∪\|∩\|∀\|∃"

" texto e comentário
syntax match algEscape "\\[nt\"\\]" contained
syntax region algString start=+"+ skip=+\\"+ end=+"+ contains=algEscape oneline
syntax match algCharacter "'\\\=.'"
syntax keyword algTodo TODO FIXME XXX contained
syntax match algComment "//.*$" contains=algTodo

highlight default link algDirective PreProc
highlight default link algLang Constant
highlight default link algConditional Conditional
highlight default link algRepeat Repeat
highlight default link algStatement Statement
highlight default link algKeyword Keyword
highlight default link algFuncName Function
highlight default link algOperatorWord Operator
highlight default link algBoolean Boolean
highlight default link algConstant Constant
highlight default link algNumber Number
highlight default link algFloat Float
highlight default link algType Type
highlight default link algBuiltin Function
highlight default link algWord Special
highlight default link algMethod Function
highlight default link algReserved Keyword
highlight default link algOperator Operator
highlight default link algEscape SpecialChar
highlight default link algString String
highlight default link algCharacter Character
highlight default link algTodo Todo
highlight default link algComment Comment

let b:current_syntax = "alg"
