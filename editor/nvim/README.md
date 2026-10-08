# hobbit.nvim

Gruntz’s Neovim integration, adapted to Hobbit’s central bindings and strict
objdiff report. Launch inside `bin/hobbit-shell`, adding this directory to
Neovim’s runtime path:

```sh
nvim --cmd 'set rtp^=editor/nvim' src/xCore/x_files/Implementation/x_plus.cpp
```

`:Hobbit target`, `base`, and `diff` show the function selected by the nearest
`RVA(...)` annotation. `status` shows the compared units; its percentage is
not whole-game reconstruction progress. `hints` toggles inline scores.
`:HobbitBuild` runs the build graph. The `vt`, `vb`, `vd`, `vs`, and `vB`
buffer mappings invoke those commands; `vq` closes views.

Optional `:Hobbit autobuild` runs `hobbit match UNIT` on save, refreshing
claims, model, delinked target, and strict comparison. Relocation checks stay
enabled in both assembly views and live scores. Outside the development
shell, builds enter it through `bin/hobbit-shell`; assembly views require
`objdiff-cli` on PATH. `:HobbitLog` shows commands and tool discovery.

Optional `:Hobbit autoformat` uses the root `.clang-format`. Autobuild and
autoformat default off. Settings persist under ignored `build/`.

```lua
require("hobbit").setup({
  keymaps = true, hints = true,
  build_on_save = false, format_on_save = false,
  split = "botright vsplit",
})
```

Inputs: `build/gen/bindings.tsv`, `build/objdiff/compare-new/report.json`, and
the normalized target/base objects in that comparison directory. The first
`hobbit build` must succeed before views can resolve source annotations.
