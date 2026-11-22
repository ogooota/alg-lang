" indentação de arquivos .alg: linha terminada em ':' abre um bloco
if exists("b:did_indent")
    finish
endif

let b:did_indent = 1

setlocal indentexpr=AlgIndent()
setlocal indentkeys=o,O,!^F

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

    if text =~ ':\s*$'
        return base + shiftwidth()
    endif

    " depois de ret / pare / continue o bloco costuma acabar
    if text =~ '^\s*\(ret\|pare\|break\|continue\)\>'
        return max([base - shiftwidth(), 0])
    endif

    return base
endfunction
