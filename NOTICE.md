# Source and license scope

The CC0 1.0 declaration in `LICENSE` applies to project-authored tooling and
documentation and to donor material already released under CC0. It does not
apply to imported game/engine source, third-party libraries, SDK headers,
compiler headers, original binaries or media.

The tooling derives from Gruntz and HoMM1. Implementations and donor revisions
are recorded in [tooling inheritance](docs/tooling-inheritance.md) and
`config/tooling-donor.json`; the retained donor license is
`scripts/DONOR-LICENSE`. The public setup follows HoMM1 at revision
`8d3ae6c96fc9b06d9c709fbdbfa181d78997b787` (README, ignore rules, input pins
and license text).

Imported Entropy source retains its original copyright notices and is
documented in [the reference policy](docs/08-sibling-entropy-source.md),
[engine source imports](docs/engine-source-imports.md) and `docs/imports/`.
Those sibling trees have no established project-wide license; publication
does not assert that they are CC0 or grant rights in them. Reconstructed
game source is not covered by this repository's tooling license.

Third-party components retain their individual terms. Preserved notices
include `docs/imports/licenses/`, inline source/header notices and the
provenance in `vendor/msvc-stl-sp5/README.md`.
