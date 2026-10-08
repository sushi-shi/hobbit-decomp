{
  description = "The Hobbit (2003, Inevitable / Entropy engine) decompilation - Linux matching build environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";

    rust-overlay = {
      url = "github:oxalica/rust-overlay/6cddd512fa2bf7231f098d3a2f92f6e4cff71e0a";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    vostok-delinker-src = {
      # The reviewed-data-topology branch (the one homm2-decomp pins): a SUPERSET of
      # the old PR#11 pin (fix/absolute-data-relocs - DIR32 for absolute data/code
      # refs, REL32 for branches), plus the data/section/contribution/reloc-alias
      # manifests the DATA-match loop needs, and it retains REAL PDB identities for
      # byte-identical function groups instead of coalescing them to synthetic names
      # (the legacy behaviour is now opt-in behind --coalesce-common-functions).
      # Measured on our own tree, same base objs: exact 2366 -> 2385 (+19).
      # Needs .idata IAT symbols in the synth PDB (synth_pdb.py emits all 456) or it
      # hard-errors on the first IAT relocation target.
      url = "github:srp-survarium/vostok-delinker/81d34b204a0384a92cf3b4c641a8430256b2922e";
      flake = false;
    };

    objdiff-src = {
      # Same release we used to download prebuilt; `objdiff-cli` is now built from
      # it so the BSS-extent patch below can apply. The GUI stays prebuilt.
      url = "github:encounter/objdiff/v3.7.3";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, rust-overlay, vostok-delinker-src, objdiff-src }:
    let
      system = "x86_64-linux";

      pkgs = import nixpkgs {
        inherit system;
        overlays = [ rust-overlay.overlays.default ];
      };

      # Nightly: the delinker uses `#![feature(os_string_truncate)]`.
      rust = pkgs.rust-bin.nightly.latest.default.override {
        extensions = [ "rust-src" "rustfmt" "clippy" ];
      };
      nightly-rustPlatform = pkgs.makeRustPlatform { cargo = rust; rustc = rust; };

      # vostok-delinker - splits the EXE into per-symbol COFF "target" objects for objdiff.
      vostok-delinker = nightly-rustPlatform.buildRustPackage {
        pname = "vostok-delinker";
        version = "0.1.0";
        src = vostok-delinker-src;
        cargoHash = "sha256-ry3TH1fz7Aj/JdbmlgQFFn29m8E7EQHyGaVXnZTEcXo=";
        # UPSTREAM-PENDING. --data-manifest rejected one rva claimed by two objects,
        # but a COMDAT is emitted into EVERY object that uses it and folded by the
        # linker onto one rva, so all owners are correct. The section manifest already
        # permits exactly this (`compatible_folded_comdat_alias`); the data manifest
        # never got the same treatment. See docs/data-attribution.md (generated manifests).
        # ILT: link.exe /INCREMENTAL routes function-ADDRESS references (vtable
        # slots, fn-ptr tables) through a 5-byte `jmp rel32` thunk band at the
        # start of .text. The thunk is a link-time artifact - cl cannot name a
        # symbol that does not exist until link, so the original object's DIR32
        # named the BODY. Resolve through the thunk to reconstruct that, the same
        # way an IAT slot is resolved back to its import. Historical evidence:
        # https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/ilt-thunk-indirection.md
        # COMDAT leader: `finish_data_comdats` demanded an external definition at
        # section offset 0, which is not what COFF means and not what cl emits.
        # Under /GR a class vtable COMDAT holds the `??_R4` complete-object-locator
        # POINTER (an unnamed word) at offset 0 and `??_7<class>@@6B@` at offset 4,
        # and the vtable symbol is that COMDAT's leader. Take the lowest-offset
        # external definition instead. See docs/data-attribution.md (COMDAT enrollment).
        # Grouped section names: the section manifest's storage check demanded an
        # exact `.rdata` / `.data` / `.bss`, with one hand-rolled exception for
        # `.CRT$`. A `$` suffix is COFF's grouped-section form (a linker ordering
        # key, stripped at link time), and cl emits every RTTI record that way -
        # `??_R1`..`??_R4` in `.rdata$r`. Compare the group prefix instead.
        # Legacy data into a COMDAT: `with_sections` adopted the FIRST manifest
        # section of each storage as the container for definitions the manifest
        # does not place, and with the candidate section manifest that is a
        # per-symbol COMDAT. A COMDAT holds exactly cl's one symbol, so the
        # appended definition (plus its alignment gap) is content the base object
        # does not have. Only an ordinary section may be the fallback.
        # Data hypothesis must CONTAIN the rva: `hypothesis_owner_and_addend_for_rva`
        # ranks enrolled definitions by `(!contains, distance, ...)` but returns the
        # best one even when NOTHING contains the rva, with an addend that is
        # unbounded in both directions - and both callers consult it BEFORE the
        # `--recover-data-relocs-from-pdb` fallback, so that guess beats an
        # exact-address PDB symbol. Measured: 1,020 of 21,730 enrolled-symbol data
        # relocations decomposed past their symbol's end, over 185 objects -
        # `??_R4CGruntVoice@@6B@ + 0x10800` into a 0x14 B RTTI locator (750 sites:
        # every /GX registration stub's `mov eax,<FuncInfo>`), `_inflate_mask +
        # 0x3db4` into 0x44 B (164), and negative addends where the nearest enrolled
        # datum sits AFTER the target. Not enrolling an rva is what the PDB fallback
        # is for. docs/build-system.md § "The EH funclet band".
        # Canonical alias owners: a reviewed reloc-alias manifest is checked in,
        # and no checked-in file may carry a volatile CodeView `$S<n>` ordinal
        # (it renumbers on any TU churn). A trailing bare `$S` marks the owner
        # as CANONICAL: match the live symbol by its ordinal-stripped spelling
        # and emit the live name. Negative addends need no change - the manifest
        # already takes two's-complement hex (`0xffffffff` = the array-1 loop
        # idiom containment can never name).
        # Unprovisioned-identity refusal: every data identity the delinker emits
        # is PROVIDED, never invented. pdb_synth seeds a fence at each reloc
        # target no real name reaches - `DAT_<va>` when only library link-bands
        # reference it (deliberately synthetic, emitted as-is), `UNPROVISIONED_
        # <va>` when any game band does. The PDB-fallback paths refuse to emit
        # an `UNPROVISIONED_` referent (bail names the rva + the remedy), and
        # the writable-statics fallback's no-symbol case bails the same way
        # instead of silently DROPPING the relocation from the emitted object.
        patches = [
          ./nix/patches/vostok-data-manifest-folded-comdat.patch
          ./nix/patches/vostok-ilt-thunk-resolution.patch
          ./nix/patches/vostok-comdat-leader-nonzero-offset.patch
          ./nix/patches/vostok-grouped-section-names.patch
          ./nix/patches/vostok-legacy-data-not-into-comdat.patch
          ./nix/patches/vostok-data-hypothesis-must-contain.patch
          ./nix/patches/vostok-canonical-alias-owner.patch
          ./nix/patches/vostok-unprovisioned-identity-refusal.patch
          ./nix/patches/vostok-fixed-manifest-iat.patch
          ./nix/patches/vostok-skip-inline-switch-tables.patch
          ./nix/patches/vostok-unpadded-function-extents.patch
        ];
      };

      # objdiff - upstream prebuilt Linux binaries (not in nixpkgs), so foreign ELF
      # patched by autoPatchelfHook. Supports x86 + COFF, our MSVC target.
      objdiffVersion = "3.7.3";
      objdiffUrl = name:
        "https://github.com/encounter/objdiff/releases/download/v${objdiffVersion}/${name}";
      objdiffGuiLibs = with pkgs; [
        libGL libxkbcommon wayland fontconfig freetype
        libx11 libxcursor libxi libxrandr libxcb
      ];

      # objdiff-cli is built FROM SOURCE (the GUI below stays a prebuilt download)
      # so that the two scoring patches below can apply.
      #
      # UPSTREAM-PENDING objdiff-bss-inferred-extent: `.bss` has no bytes, so a BSS
      # symbol's size is objdiff's whole comparison - but COFF encodes a symbol size
      # only for a COMMON symbol, so `infer_symbol_sizes` synthesises one from the
      # distance to the next symbol: the object PLUS its allocator's padding. Our two
      # sides use different allocators (cl for the base, the delinker's aligned append
      # for the target), so that span differs for reasons that are not the program.
      # Census over the whole tree: 364 paired `.bss` symbols, 51 disagreements, every
      # single delta 3/4/6 bytes - all sub-alignment padding, none a real size. The
      # extent audit that DOES bite lives in `gruntz.build.data_manifest` (a reviewed
      # extent must fit the span to its retail neighbour, or both rows are withheld).
      # Upstream cannot relax this globally: for a format that DOES state sizes the
      # comparison is real. Drop the patch if objdiff stops comparing INFERRED sizes.
      # https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/bss-symbol-size-inference-hole.md.
      #
      # UPSTREAM-PENDING objdiff-score-reloc-addend: x86 COFF `DIR32` has no addend
      # field - the addend sits in the instruction's displacement, which is exactly
      # the operand the diff masks as "relocated". objdiff recovers it, and the `all`
      # / `name_address` reloc modes do compare it, but `data_value` (what we use)
      # short-circuits that clause and drops the addend along with the name. So
      # `g_clut + 0x20000` vs `g_clut + 0x1fffe` scored IDENTICAL - the one operand
      # class where a wrong constant is free, and it hid a live off-by-one across a
      # 3 x 32768-entry LUT (commit 61d15c531). The patch compares the addend when
      # both sides resolve to the SAME symbol and the reloc type's addend is a plain
      # symbol offset (a new `Arch` predicate, default false; REL32 keeps its current
      # behaviour since its stored value is site-relative). Upstream does not do it
      # because `data_value` exists for projects with unreliable target symbol names,
      # where the pointed-to VALUE is the trusted signal - nobody had noticed the
      # addend is a third signal that survives unreliable names. Drop the patch once
      # objdiff scores addends under `data_value` (or grows an addend knob).
      # https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/reloc-addend-is-masked-diff-the-addends.md.
      objdiff-cli = nightly-rustPlatform.buildRustPackage {
        pname = "objdiff-cli";
        version = objdiffVersion;
        src = objdiff-src;
        patches = [
          ./nix/patches/objdiff-bss-inferred-extent.patch
          ./nix/patches/objdiff-score-reloc-addend.patch
        ];
        cargoHash = "sha256-Z9vyUj35nrHuUoOYM54RLCn7CzcQ6k3A6FsDYKCVqVM=";
        cargoBuildFlags = [ "-p" "objdiff-cli" ];
        cargoTestFlags = [ "-p" "objdiff-core" "-p" "objdiff-cli" ];
        cargoInstallFlags = [ "-p" "objdiff-cli" ];
      };

      objdiff = pkgs.stdenv.mkDerivation {
        pname = "objdiff";
        version = objdiffVersion;
        src = pkgs.fetchurl {
          url = objdiffUrl "objdiff-linux-x86_64";
          hash = "sha256-1pzhzJUl/BJQP2XS333KIfkx1YYi8ZyRdPMv5MnJGyA=";
        };
        dontUnpack = true;
        nativeBuildInputs = [ pkgs.autoPatchelfHook pkgs.makeWrapper ];
        buildInputs = [ pkgs.stdenv.cc.cc.lib ] ++ objdiffGuiLibs;
        installPhase = ''
          install -Dm755 $src $out/bin/objdiff
          wrapProgram $out/bin/objdiff \
            --prefix LD_LIBRARY_PATH : "${pkgs.lib.makeLibraryPath objdiffGuiLibs}"
        '';
      };

      hobbit-toolchain = import ./nix/toolchain.nix { inherit pkgs; };
      hobbit-cli = pkgs.writeShellScriptBin "hobbit" ''
        d="''${HOBBIT_DIR:-}"
        if [ -z "$d" ]; then
          p="$PWD"
          while [ "$p" != "/" ]; do
            if [ -f "$p/scripts/hobbit/cli.py" ]; then d="$p"; break; fi
            p="$(dirname "$p")"
          done
        fi
        if [ ! -f "$d/scripts/hobbit/cli.py" ]; then
          echo "hobbit: run inside the checkout or set HOBBIT_DIR" >&2
          exit 2
        fi
        export HOBBIT_DIR="$d"
        export PYTHONPATH="$d/scripts''${PYTHONPATH:+:$PYTHONPATH}"
        exec python3 -m hobbit "$@"
      '';
      hobbitShell = pkgs.mkShell {
        name = "hobbit-decomp";
        packages = [ hobbit-cli rust objdiff objdiff-cli vostok-delinker ] ++ (with pkgs; [
          (python3.withPackages (ps: [ ps.pyghidra ps.libclang ps.pefile ps.capstone ]))
          ghidra ninja llvm llvmPackages.clang-unwrapped ripgrep file xxd jq
          binutils gdb wineWow64Packages.staging jdk21 p7zip cabextract
        ]);
        shellHook = ''
          _hobbit_root="$PWD"
          while [ "$_hobbit_root" != "/" ] && [ ! -f "$_hobbit_root/scripts/hobbit/cli.py" ]; do
            _hobbit_root="$(dirname "$_hobbit_root")"
          done
          if [ "$_hobbit_root" = "/" ]; then
            echo "[hobbit] enter this shell from a Hobbit checkout" >&2
          else
            export HOBBIT_DIR="$_hobbit_root"
          fi
          unset _hobbit_root
          export HOBBIT_EXE="$HOBBIT_DIR/build/orig/Meridian.exe"
          export HOBBIT_CLANG="${pkgs.llvmPackages.clang-unwrapped}/bin/clang"
          export LIBCLANG_PATH="${pkgs.llvmPackages.libclang.lib}/lib"
          export PYTHONPATH="$HOBBIT_DIR/scripts''${PYTHONPATH:+:$PYTHONPATH}"
          export WINEPREFIX="$HOBBIT_DIR/build/wineprefix"
          export WINEDEBUG="fixme-all,err-kerberos"
          export WINEDLLOVERRIDES="mscoree,mshtml="
          case "$-" in *i*) trap 'wineserver -k >/dev/null 2>&1 || true' EXIT ;; esac
          export HOBBIT_TOOLCHAIN="${hobbit-toolchain}"
          export MSVC_DIR="${hobbit-toolchain}/msvc"
          export GHIDRA_INSTALL_DIR="${pkgs.ghidra}/lib/ghidra"
          export JAVA_HOME="${pkgs.jdk21}/lib/openjdk"
          echo "[hobbit] matching shell: VC6, Wine, patched objdiff/vostok, llvm-pdbutil, clang and Ghidra" >&2
          if [ -z "''${HOBBIT_SKIP_INIT:-}" ]; then
            python3 -m hobbit init || echo "[hobbit] init failed; inspect diagnostics and rerun hobbit init" >&2
          fi
        '';
      };
    in {
      packages.${system} = {
        inherit vostok-delinker objdiff objdiff-cli hobbit-toolchain;
        default = vostok-delinker;
      };
      devShells.${system} = { default = hobbitShell; build = hobbitShell; };
    };
}
