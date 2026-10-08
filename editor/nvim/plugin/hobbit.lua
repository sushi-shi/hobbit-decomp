-- hobbit.nvim - commands + buffer-local keymaps (see lua/hobbit/init.lua).
if vim.g.loaded_hobbit then return end
vim.g.loaded_hobbit = true

local hobbit = require("hobbit")

vim.api.nvim_create_user_command("Hobbit", function(o)
  hobbit.dispatch(o.fargs[1])
end, {
  nargs = "?",
  complete = function() return hobbit.complete() end,
  desc = "hobbit: {target|base|diff|status} asm/diff for the function at cursor",
})

vim.api.nvim_create_user_command("HobbitBuild", function(o)
  hobbit.build(o.fargs)
end, { nargs = "*", desc = "hobbit: recompile (MSVC+wine) and report what moved" })

vim.api.nvim_create_user_command("HobbitLog", function()
  hobbit.show_log()
end, { desc = "hobbit: log of objdiff/build invocations" })

-- Attach the chords + the missing-tool warning + inline % hints on C/C++ buffers
-- only; outside a hobbit checkout the plugin stays inert (attach_keymaps is
-- harmless, check is silent, hints early-return with no project root).
local grp = vim.api.nvim_create_augroup("hobbit", { clear = true })

vim.api.nvim_create_autocmd("FileType", {
  pattern = { "c", "cpp" },
  group = grp,
  callback = function(ev)
    hobbit.load_state(ev.buf) -- restore this checkout's remembered toggles first
    if hobbit.config.keymaps then hobbit.attach_keymaps(ev.buf) end
    hobbit.check(ev.buf)
    hobbit.hints(ev.buf)
  end,
})

-- Refresh the inline % hints on enter/save (a build elsewhere may have moved the
-- numbers; report.json is mtime-cached so this is cheap).
vim.api.nvim_create_autocmd({ "BufWinEnter", "BufEnter", "BufWritePost" }, {
  pattern = { "*.c", "*.cpp", "*.cc", "*.h", "*.hpp" },
  group = grp,
  callback = function(ev) hobbit.hints(ev.buf) end,
})

-- Format-on-save (off by default; `:Hobbit autoformat` toggles): clang-format the
-- saved file in place before it hits disk, so one save writes formatted source.
-- BufWritePre (not Post) so the formatting is part of the write, not a reload.
vim.api.nvim_create_autocmd("BufWritePre", {
  pattern = { "*.c", "*.cpp", "*.cc", "*.cxx", "*.h", "*.hpp", "*.hh" },
  group = grp,
  callback = function(ev) hobbit.format_on_save(ev.buf) end,
})

-- Build-on-save (off by default; `:Hobbit autobuild` toggles): a quiet
-- incremental rebuild on every TU save so the inline %s update as you edit.
vim.api.nvim_create_autocmd("BufWritePost", {
  pattern = { "*.c", "*.cpp", "*.cc" },
  group = grp,
  callback = function(ev) hobbit.on_save(ev.buf) end,
})

-- Initial render for buffers already open when the plugin loads. When nvim is
-- launched on a file (e.g. via the dev-shell wrapper's `--cmd "set rtp^=…"`),
-- that buffer's FileType/BufEnter can fire before this autocmd is registered, so
-- the first render would be missed without this sweep. (hints early-returns for
-- non-project / non-source buffers.)
vim.schedule(function()
  for _, b in ipairs(vim.api.nvim_list_bufs()) do
    if vim.api.nvim_buf_is_loaded(b) then hobbit.hints(b) end
  end
end)
