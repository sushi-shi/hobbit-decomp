"""hobbit.core.msvc_names - one spelling for one VC6 symbol.

Two directions over the same vocabulary:

  * FORWARD (labelling). clang proposes a mangled name; VC6's own spelling
    for the same declaration is a DETERMINISTIC function of that name plus the
    declaration's linkage, so `func`/`data` derive it from SOURCE alone - no
    object is read, and a stale build artifact can no longer answer for source
    that has changed. Three rules, each measured over the whole claim corpus
    (`hobbit verify selftest -k SourceNameRewrite` re-proves them per build):
      1. the i386 COFF global prefix - LLVM adds `_` to every name that is not
         already MSVC-mangled (`?...`); the IR value name lacks it, libclang's
         `mangledName` already carries it;
      2. array storage class - clang can spell a nonmember array `@@<d>Q`
         (const pointer), VC6 `@@<d>P`. Static class-member arrays (access
         digits 0, 1, 2) retain Q when their elements are const;
      3. VC6 file-static symbols retain their ordinary decorated identifier.
         Unlike the Gruntz VC5 donor they do not acquire a _$S wrapper.
         A compiler fixture covers file statics, consts, arrays and locals.


  * MASK (joining). The VC5 donor stamps a per-object CodeView counter onto
    TU-local data (`name$S<n>`); VC6 retains plain identifiers and numbers a
    function-local static's enclosing lexical
    scope (`@?<n>??`, `<n>` counting every scope c1 opened). Both renumber on
    any edit to the translation unit, so neither is ever stored: `mask` reduces
    EITHER side of a name join to the ordinal-free form, which is what makes a
    canonical claim and cl's own object symbol meet.

`discriminate` is the one sanctioned way back to a unique spelling: several
translation units can hold a same-named TU-local static, and the delink data
manifest needs one name per address image-wide. The retail rva is the
discriminator - the convention cl-generated data already uses (`$T<rva>` FP
pool slots), and `mask` folds it straight back onto the family.
"""

from __future__ import annotations

import hashlib
import re

#: Nonmember array storage only. VC6 itself emits @@[012]QB for const
#: static-member arrays; rewriting those changes the real symbol identity.
ARRAY_STORAGE = re.compile(r"@@([3-9])Q")
#: cl's per-object CodeView counter on a TU-local datum. Every occurrence is
#: volatile, not just the trailing one (a function-local static's guard byte is
#: spelled `?$S<n>@?<scope>??<fn>@4EA$S<n>`). A name that is NOTHING BUT `$S<n>`
#: is not this: that is the rva-keyed spelling `discriminate` produces.
STATIC_ORDINAL = re.compile(r"(?<=.)\$S[0-9]+(?=@|$)")
#: cl's lexical-scope number in a function-local static's mangled name. MSVC
#: spells 1..10 as the digits `0`..`9` and larger values in hex as `A..P@`.
LOCAL_STATIC_SCOPE = re.compile(r"@\?(?:[0-9]|[A-P]+@)\?\?")
#: the scope spelling both sides agree on - clang's, for the one scope we model.
CANONICAL_SCOPE = "@?1??"
# VC5 encodes a source-file anonymous namespace as ?%<absolute cpp path><nonce>@.
# Only a namespace declared in a repository .cpp can be identified from this
# spelling alone. A header path does NOT identify its instantiating TU.
ANONYMOUS_NAMESPACE = re.compile(r"\?%([^@]+?\.(?:cpp|cxx|cc))([0-9]+)@")


def anonymous_namespaces(name: str) -> str:
    """Stable TU identity for source-file anonymous namespaces, never headers.

    Keep the source path below src/, discarding only the checkout prefix and
    compiler nonce. Standard MSVC anonymous-namespace syntax carries the hash.
    This is a comparison/claim spelling; the linker consumes the untouched obj.
    """
    def replace(match: re.Match) -> str:
        path = re.sub(r"/+", "/", match[1].replace("\\", "/"))
        before, separator, relative = path.rpartition("/src/")
        if not separator:
            return match[0]
        identity = "src/" + relative
        digest = hashlib.sha256(identity.encode("utf-8")).hexdigest()[:16]
        return "?A0x" + digest + "@"
    return ANONYMOUS_NAMESPACE.sub(replace, name)


def decorate(name: str) -> str:
    """The i386 COFF global prefix LLVM applies to a name it did not mangle."""
    return name if name.startswith("?") else "_" + name


VC6_LOCAL_RVA = re.compile(r"\$HOBBIT_LOCAL_[0-9a-f]{8}$")


def local_source_name(name: str) -> str:
    """Remove only the model-produced discriminator for a proved VC6 local."""
    return VC6_LOCAL_RVA.sub("", name)


def mask(name: str) -> str:
    """`name` with every volatile cl ordinal reduced to its canonical form."""
    return LOCAL_STATIC_SCOPE.sub(
        CANONICAL_SCOPE, STATIC_ORDINAL.sub("$S", anonymous_namespaces(local_source_name(name))))


def func(name: str, *, decorated: bool = False) -> str:
    """VC6's spelling for a clang-proposed FUNCTION name."""
    out = ARRAY_STORAGE.sub(r"@@\1P", name)
    return mask(out if decorated else decorate(out))


def data(name: str, *, internal: bool, decorated: bool = False) -> str:
    """VC6 spelling derived from a Clang name, without reading compiled claims.

    ``internal`` remains in the common extraction API, but VC6 does not wrap
    internal globals in VC5's _$S form. Local-static scope ordinals still pass
    through the shared mask. Proved with build/probes/static_names.cpp on
    compiler 12.00.9044 (see docs/vc6-static-symbols.md).
    """
    out = ARRAY_STORAGE.sub(r"@@\1P", name)
    if not decorated:
        out = decorate(out)
    return mask(out)


def discriminate(name: str, rva: int) -> str | None:
    """`name` respelled for one rva, or None when the family has no room.

    Only the `$S` family carries a discriminator: the ordinal slot cl fills
    with a per-object counter takes the retail rva instead, so the spelling is
    unique image-wide and `mask` still folds it onto the shared family.
    """
    return f"{name}{rva}" if name.endswith("$S") else None
