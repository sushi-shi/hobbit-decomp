# Microsoft Visual C++ 6 SP5 XTREE

The opt-in header lives at `include/XTREE`; pass an explicit include path to
that nested directory when a reviewed unit profile calls for SP5. Vendor roots
are automatically searched, so no header sits directly at this vendor root.

This is the unchanged period header recovered from the local HoMM1 preflight
SP5 media. It is an opt-in include overlay, not an active global toolchain change.
The other headers must still come from the reviewed VC6 header closure.

Donor project revision: `efdfdf0fb0df55ad589249e657d37c7a8f058367`.
Original media `vs6sp5.exe` SHA-256:
`10a834041223ad21181c0e87c0a90930d976498556af2e8f959a044101e6a604`.
Unchanged XTREE SHA-256:
`d0280d2cb8c2b2d14ce60dc868da5159adf1f5844a971d623883ffba4da35354`.
Header revision timestamp: 2000-08-23 18:36:10; size 30430 bytes.

Local origin:
`/home/sheep/Projects/homm1/homm1-decomp-buka/build/preflight/vc6-media/sp5/vc98/include/xtree`.
Fresh extraction from all ten cabinets in the hash-pinned official media gives
the same hash. Scratch provenance, cabinet hashes, header closure, exact
176-byte PC set-constructor proof and references are preserved in
`build/probes/cohort12_admissions/d3dfull/period_header_package.json` and
`set_sp5_proof.json`.

SP5 allocates its temporary sentinel before acquiring `_Lockit`; the base VC6
header acquires `_Lockit` first. The authentic SP5 implementation independently
matches the Hobbit PC constructor. This establishes a header variant for that
consumer, not complete TU storage layout or a universal profile for all units.
