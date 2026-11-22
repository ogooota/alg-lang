-- tema "alg": escuro e genérico, pensado para pseudocódigo (vamos reformulando)
vim.cmd("highlight clear")

if vim.fn.exists("syntax_on") == 1 then
    vim.cmd("syntax reset")
end

vim.o.background = "dark"
vim.g.colors_name = "alg"

local palette = {
    background = "#16161e",
    surface = "#1f2029",
    line = "#262735",
    selection = "#33344a",
    text = "#d8d6e0",
    muted = "#6c6f85",
    keyword = "#c792ea",
    flow = "#ff9e64",
    builtin = "#7fd1c7",
    method = "#82aaff",
    type = "#ffcb6b",
    number = "#f78c6c",
    string = "#c3e88d",
    constant = "#ff5370",
    directive = "#89ddff",
    operator = "#a9b1d6",
}

local function set(group, spec)
    vim.api.nvim_set_hl(0, group, spec)
end

-- interface
set("Normal", { fg = palette.text, bg = palette.background })
set("NormalFloat", { fg = palette.text, bg = palette.surface })
set("CursorLine", { bg = palette.line })
set("CursorLineNr", { fg = palette.flow, bold = true })
set("LineNr", { fg = palette.muted })
set("Visual", { bg = palette.selection })
set("Search", { fg = palette.background, bg = palette.type })
set("IncSearch", { fg = palette.background, bg = palette.flow })
set("Pmenu", { fg = palette.text, bg = palette.surface })
set("PmenuSel", { fg = palette.background, bg = palette.method })
set("StatusLine", { fg = palette.text, bg = palette.surface })
set("StatusLineNC", { fg = palette.muted, bg = palette.surface })
set("VertSplit", { fg = palette.line })
set("WinSeparator", { fg = palette.line })
set("SignColumn", { bg = palette.background })
set("MatchParen", { fg = palette.flow, bold = true, underline = true })
set("NonText", { fg = palette.line })

-- sintaxe genérica
set("Comment", { fg = palette.muted, italic = true })
set("String", { fg = palette.string })
set("SpecialChar", { fg = palette.directive })
set("Number", { fg = palette.number })
set("Float", { fg = palette.number })
set("Boolean", { fg = palette.constant })
set("Constant", { fg = palette.constant, bold = true })
set("Identifier", { fg = palette.text })
set("Function", { fg = palette.method })
set("Statement", { fg = palette.keyword })
set("Conditional", { fg = palette.flow, bold = true })
set("Repeat", { fg = palette.flow, bold = true })
set("Keyword", { fg = palette.keyword })
set("Operator", { fg = palette.operator })
set("PreProc", { fg = palette.directive, bold = true })
set("Type", { fg = palette.type })
set("Special", { fg = palette.type, italic = true })
set("Todo", { fg = palette.background, bg = palette.type, bold = true })
set("Error", { fg = palette.constant, undercurl = true })

-- grupos da linguagem alg
set("algBuiltin", { fg = palette.builtin })
set("algMethod", { fg = palette.method })
set("algFuncName", { fg = palette.method, bold = true })
set("algOperatorWord", { fg = palette.keyword, italic = true })
set("algStatement", { fg = palette.keyword, bold = true })
set("algReserved", { fg = palette.muted, italic = true })
