"""Locally rebuild the ICO container from the user's pinned PC executable.

Adapted from HoMM1 scripts/homm1/tool/rc.py:icon_container and
clean/template/build.py:icon_group at 8d3ae6c96fc9b06d9c709fbdbfa181d78997b787.
Hobbit has nine images, one named group and language 1033. No payload is
distributed with this module; the generated files belong under build/.
"""

from pathlib import Path
import struct

from hobbit.core.inputs import _verify, targets
from hobbit.core.pe import Pe
from hobbit.rsrc.res import read_pe_rsrc


def verified_resources(exe=None):
    pe = Pe(exe)
    pin = targets()['game']
    # Explicit --exe paths must pass the same pin as the default input. Verify
    # the bytes actually parsed, rather than reopening a potentially changed file.
    _verify(pe.data, pe.path, size=pin.size, sha256=pin.sha256)
    return read_pe_rsrc(pe)


def icon_container(retail):
    groups = [row for row in retail if row[0] == 14]
    if len(groups) != 1 or groups[0][1:4] != ('IDC_ICON1', 1033, 0):
        raise ValueError('expected one IDC_ICON1 group, language 1033, codepage 0')
    group = groups[0][4]
    if len(group) != 6 + 14 * 9 or struct.unpack_from('<HHH', group) != (0, 1, 9):
        raise ValueError('expected nine images in the IDC_ICON1 group')
    images = {}
    for kind, name, language, codepage, data in retail:
        if kind == 3:
            if language != 1033 or codepage or name in images:
                raise ValueError('unexpected or duplicate icon image identity')
            images[name] = data
    header, payloads = bytearray(group[:6]), bytearray()
    used = []
    for index in range(9):
        entry = group[6 + 14 * index:6 + 14 * (index + 1)]
        size, ordinal = struct.unpack_from('<IH', entry, 8)
        image = images.get(ordinal)
        if image is None or len(image) != size or ordinal in used:
            raise ValueError(f'RT_ICON {ordinal} does not match its group entry')
        header.extend(entry[:12] + struct.pack('<I', 6 + 16 * 9 + len(payloads)))
        payloads.extend(image)
        used.append(ordinal)
    if set(images) != set(used):
        raise ValueError('icon images exist outside IDC_ICON1')
    return bytes(header + payloads)


def stage_script(source, directory, retail):
    """Stage the resource script and local payload together for RC.EXE."""
    payload = icon_container(retail)
    directory = Path(directory)
    (directory / 'res').mkdir(parents=True, exist_ok=True)
    (directory / 'res/meridian.ico').write_bytes(payload)
    script = directory / Path(source).name
    script.write_bytes(Path(source).read_bytes())
    return script
