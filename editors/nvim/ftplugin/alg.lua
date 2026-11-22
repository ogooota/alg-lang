-- opções de edição para arquivos .alg
if vim.b.did_ftplugin then
    return
end

vim.b.did_ftplugin = true

-- blocos são por indentação: 4 espaços, nunca tab
vim.opt_local.expandtab = true
vim.opt_local.tabstop = 4
vim.opt_local.shiftwidth = 4
vim.opt_local.softtabstop = 4

-- comentário é //
vim.opt_local.commentstring = "// %s"
vim.opt_local.comments = "://"

vim.b.undo_ftplugin = "setlocal expandtab< tabstop< shiftwidth< softtabstop< commentstring< comments<"
