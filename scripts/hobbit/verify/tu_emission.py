"""Verify actual source-owned COFF emission order and build freshness.

Derives section ordering from local Gruntz's documented contribution-arrival
contract; does not import its VC5 assumption that source order equals emission.
Donor: local Gruntz7d4bd55b99e32f084834d991badf7609889481f4,
verify/tu_order.py and graph's Scanner/Ninja/restat freshness contract.
Measured Hobbit VC6 adapter: qualified .debug$F SECREL function records give
current compiler body extents, distinct from retail spans. This extends the
local Gruntz ownership/bounds contract and HoMM1 compiler-frame audit policy;
no external tool donor or comparison/exactness rule is introduced.
"""
import struct
from hobbit.compare.canonicalize import CoffObject, MEM_EXECUTE, FUNCTION_TYPE
from hobbit.core.msvc_names import mask


def order_unit(unit, entries, claims, coff):
    positions = []
    section_headers_end = max((s.header_offset + 40 for s in coff.sections), default=20)
    for entry in entries:
        if entry.size <= 0:
            raise ValueError(f'{unit}: {entry.rva:#x} requires a positive complete extent')
        candidates = set()
        for claim in claims:
            if (claim.unit != unit or claim.kind != 'func' or claim.channel != 'src'
                    or claim.rva != entry.rva or claim.size != entry.size):
                continue
            for symbol in coff.symbols.values():
                if (mask(symbol.name) != mask(claim.name) or symbol.section <= 0
                        or symbol.typ != FUNCTION_TYPE or symbol.storage_class not in (2,3)):
                    continue
                if symbol.section > len(coff.sections):
                    raise ValueError(f'{unit}: {claim.name} has an invalid COFF section index')
                section = coff.sections[symbol.section - 1]
                if section.characteristics & (MEM_EXECUTE | 0x20) != (MEM_EXECUTE | 0x20):
                    raise ValueError(f'{unit}: {claim.name} is not stored executable code')
                if (section.raw_offset <= 0 or section.raw_offset < section_headers_end
                        or section.raw_offset + section.raw_size > min(len(coff.data), coff.symbol_offset)):
                    raise ValueError(f'{unit}: {claim.name} has no bounded stored code payload')
                # Target span and current compiler body can legitimately differ.
                # Bound the actual emitted function using its qualified debug$F
                # record, never the retail length or an unbounded section guess.
                emitted_lengths = []
                for relocation in coff.relocations:
                    if not 1 <= relocation.section <= len(coff.sections):
                        raise ValueError(f'{unit}: invalid relocation section index')
                    debug = coff.sections[relocation.section - 1]
                    if debug.name != '.debug$F':
                        continue
                    referent = coff.symbols.get(relocation.symbol_index)
                    if referent is None:
                        raise ValueError(f'{unit}: missing emitted extent relocation referent')
                    if referent.name != symbol.name:
                        continue
                    if (relocation.typ != 7 or referent.section != symbol.section
                            or referent.value != symbol.value or referent.typ != FUNCTION_TYPE):
                        raise ValueError(f'{unit}: {claim.name} has wrong emitted extent referent or relocation type')
                    if (debug.raw_offset < section_headers_end or debug.raw_size < 8
                            or debug.raw_offset + debug.raw_size > min(len(coff.data), coff.symbol_offset)
                            or relocation.site != 0):
                        raise ValueError(f'{unit}: {claim.name} has malformed emitted extent evidence')
                    emitted_lengths.append(struct.unpack_from('<I', coff.data, debug.raw_offset + 4)[0])
                if not emitted_lengths:
                    # Some measured VC6 COMDAT producers omit debug$F frames.
                    # Retain exactly the existing target-fit stored-position
                    # check; this does not establish a current body length.
                    emitted_extent = entry.size
                elif len(emitted_lengths) != 1 or emitted_lengths[0] <= 0:
                    raise ValueError(f'{unit}: {claim.name} needs one positive compiler emitted extent')
                else:
                    emitted_extent = emitted_lengths[0]
                if symbol.value < 0 or symbol.value + emitted_extent > section.raw_size:
                    raise ValueError(f'{unit}: {claim.name} does not fit emitted section')
                candidates.add((section.name, symbol.section, symbol.value, symbol.name))
        if len(candidates) != 1:
            raise ValueError(f'{unit}: {entry.rva:#x} needs one exact emitted source identity, got {len(candidates)}')
        position = next(iter(candidates))
        positions.append((position, entry))
    # Do not silently apply a VC5 group-order rule to unmeasured VC6 groups.
    # Current actual source functions all emit into the one .text group.
    if {p[0] for p,_e in positions} - {'.text'}:
        raise ValueError(f'{unit}: multiple/unreviewed code section groups require layout evidence')
    locations = [(p[1], p[2]) for p,_e in positions]
    if len(set(locations)) != len(locations):
        raise ValueError(f'{unit}: shared emitted position requires explicit folded-identity proof')
    return [entry for _position,entry in sorted(positions, key=lambda row:row[0][1:3])], positions


import os
from pathlib import Path
import shutil
import subprocess


def require_built(root, graph_file, targets, *, ninja=None):
    root=Path(root)
    for name in (graph_file, 'build/.ninja_log', *targets):
        if not (root/name).is_file():
            raise ValueError(f'missing build evidence {name}; run hobbit build')
    executable=ninja or shutil.which('ninja')
    if not executable:
        raise ValueError('ninja unavailable; enter the matching shell and run hobbit build')
    result=subprocess.run([str(executable),'-n','-f',str(graph_file),*targets],
                          cwd=root,env=dict(os.environ,NINJA_STATUS='TU_ORDER_PENDING '),
                          text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if result.returncode or result.stdout.strip()!='ninja: no work to do.' or result.stderr.strip():
        detail=(result.stderr or result.stdout).strip().splitlines()
        raise ValueError('stale or invalid build evidence; run hobbit build: '+
                         ('; '.join(detail[:4]) or f'ninja rc={result.returncode}'))


def require_current_units(units):
    from hobbit.core.paths import REPO
    from hobbit.graph import NINJA,BASE_DIR,CLAIMS_DIR,TOOLCHAIN_ID
    from hobbit.graph.verbs import toolchain_repinned
    if not (REPO/TOOLCHAIN_ID).is_file() or toolchain_repinned():
        raise ValueError('toolchain identity differs from compiled evidence; run hobbit build in the matching shell')
    targets=[path for unit in sorted(units)
             for path in (f'{BASE_DIR}/{unit}.obj',f'{CLAIMS_DIR}/{unit}.tsv')]
    require_built(REPO,NINJA,targets)
