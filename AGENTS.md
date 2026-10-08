# Hobbit decompilation

Reconstruct evidence-backed C++ for The Hobbit. The target executable's bytes
decide behavior, layout, calling conventions, and matching correctness.

## Public repository

Keep original executables, original maps, game media and acquisition links out
of every commit. Inputs and generated media belong under ignored `build/`;
retain full hashes and evidence without download locations. Run
`python3 scripts/check_public_tree.py --staged` before committing and the
same command without `--staged` before pushing. See
`docs/public-repository.md` and `docs/reference-binaries.md`.

## Tooling and workflow

- Derive tooling and approaches from the local `/home/sheep/Projects/gruntz/`
  and `/home/sheep/Projects/homm1/` projects, as requested by the user. Adapt
  their existing mechanisms to Hobbit; record which donor implementation and
  revision each port uses. Earlier reports suggesting other tooling donors do
  not override this instruction.
- Follow Gruntz's source ownership model: `config/units.toml` maps units to
  source paths and complete compiler flag profiles; `config/retail/` holds
  admitted binary facts; source annotations provide reconstructed identities;
  generated claims and bindings belong in ignored `build/`.
- Recover original translation units and library grouping from evidence.
  Shared classes have one definition in a shared header. Do not manufacture
  source bodies, types, padding, or ownership to fill an inventory.
- Derive compiler settings from Hobbit evidence. Gruntz's VC5 and HoMM1's VC4
  profiles are not Hobbit settings.
- Parallel workers are authorized. Give them distinct responsibilities and
  preserve concurrent changes.

## Evidence

- Pin each input executable by full hash before admitting address claims.
  Keep PC RVAs, PC virtual addresses, Xbox map addresses, and XBE addresses
  distinct. Xbox names and object membership do not establish PC addresses.
- The PC reference executable, Xbox executable, and Xbox map are recovered
  under ignored `build/orig/`; their identities are pinned in
  `config/retail/targets.json`. Older reports refer to a missing `target/`.
  Treat other historical measurements as documented findings until checked.
- The Xbox `.map` is known evidence. A game PDB has not been established, and
  the user clarified that their prior work processed the map.
- Area 51 and Tribes: Aerial Assault sources under
  `/home/sheep/Projects/archive/entropy-src/` are sibling-engine references.
  Check their layouts and behavior against Hobbit. Follow the existing local
  reference policy in `docs/08-sibling-entropy-source.md`.
- The Kingjoyer SDK supplies community mapping leads. Preserve its revision,
  target-build provenance, and uncertainty; generated stubs and approximate
  layouts are not verified reconstruction claims.
- Debug/context strings can identify inlined helpers. A `LockObject` marker
  inside a caller does not establish a new function boundary or transfer
  ownership of the helper to the caller's translation unit.
- Account for platform code generation differences, including the reported
  Xbox SSE versus PC x87 math. Match names using supporting structural
  evidence; do not expect instruction identity across builds.

See `docs/source-mapping.md` for the donor contracts and admission rules.

## Matching loop and scores

Current user priority: import the available engine source first, then polish
matching. Earlier CUR scores may fall as shared source changes; recover them
later using the normal CUR <= MAX <= HIST ledger. Do not require old matches
or every imported function to stay at 100% before integrating a larger source
family. Temporary compile and match failures are allowed during this phase
and must remain visible. Preserve real source ownership and provenance, and
admit PC addresses only when independently supported. Unmapped original
bodies may be imported without address annotations.

All helper agents must use `gpt-6.1-sol`.

Use `bin/hobbit-shell` while the environment files remain untracked, then
`hobbit build` or `hobbit match UNIT`. Source changes must refresh claims and
the target model before comparison. Keep `functionRelocDiffs=all`; byte-only
or relocation-disabled scores are diagnostic, never evidence of exactness.

`hobbit verify bank` records CUR (latest score), MAX (best score for the current
source hash), and HIST (all-time best). Editing a function resets its MAX to
its new CUR; HIST is retained. Thus CUR <= MAX <= HIST. The README's generated
status covers admitted reconstruction targets and reports unresolved text
separately. The editor's compared-unit percentage is not whole-game progress.

Run applicable verification gates before handing off changes. Report genuine
unreconstructed coverage findings separately from tool failures; never add
exemptions just to make a partial reconstruction pass a whole-game gate.
The initial x_plus profile's `/G6` is experimentally supported for this unit,
not a universal compiler setting for every future unit. Full executable linking
needs a reviewed link profile and sufficient reconstructed source.
