" indentação de arquivos .alg: blocos entre chaves
if exists("b:did_indent")
    finish
endif

let b:did_indent = 1

setlocal indentexpr=AlgIndent()
setlocal indentkeys=o,O,0},!^F

let b:undo_indent = "setlocal indentexpr< indentkeys<"

if exists("*AlgIndent")
    finish
endif

function! AlgIndent()
    let previous = prevnonblank(v:lnum - 1)

    if previous == 0
        return 0
    endif

    " ignora comentário no fim da linha anterior
    let text = substitute(getline(previous), '\s*//.*$', '', '')
    let base = indent(previous)

    " linha anterior abre bloco: entra um nível
    if text =~ '{\s*$'
        let base += shiftwidth()
    endif

    " linha atual fecha bloco: volta um nível
    if getline(v:lnum) =~ '^\s*}'
        let base -= shiftwidth()
    endif

    return max([base, 0])
endfunction
