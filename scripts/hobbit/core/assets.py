"""Generate private media includes from explicitly supplied, hash-pinned inputs.

Follows HoMM1's local executable resource extraction in
scripts/homm1/clean/template/source/build.py, revision
efdfdf0fb0df55ad589249e657d37c7a8f058367, and reuses the Gruntz-derived
PE reader (7d4bd55b99e32f084834d991badf7609889481f4) and HoMM1 input pins.
Hobbit bitmap RVAs and sibling source hashes are independent evidence.
"""
from __future__ import annotations

import argparse
import ast
import hashlib
import json
from pathlib import Path
import re
import sys
import tempfile

from hobbit.core.inputs import InputError, read_verified, targets
from hobbit.core.paths import REPO, retail_exe
from hobbit.core.pe import Pe


def _manifest(root: Path) -> dict:
    return json.loads((root / 'config/retail/local-assets.json').read_text())


def _initializer(data: bytes, spec: dict) -> bytes:
    width, count = spec['width'], spec['count']
    if len(data) != width * count or hashlib.sha256(data).hexdigest() != spec['sha256']:
        raise InputError(f"{spec['file']}: media bytes differ from pinned payload")
    values = [int.from_bytes(data[i:i + width], 'little') for i in range(0, len(data), width)]
    columns = 8 if width == 4 else 16
    lines = ['// Generated locally from hash-verified input; do not publish.']
    lines += ['    ' + ', '.join(f'0x{v:0{width * 2}x}' for v in values[i:i + columns]) + ','
              for i in range(0, count, columns)]
    return ('\n'.join(lines) + '\n').encode()


def _install(root: Path, payloads: list[tuple[dict, bytes]]) -> list[Path]:
    # Validate every array before touching the output tree.
    prepared = [(root / 'build/gen/retail-assets' / spec['file'], _initializer(data, spec))
                for spec, data in payloads]
    for path, content in prepared:
        if path.is_file() and path.read_bytes() == content:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = None
        try:
            with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
                temporary = Path(stream.name)
                stream.write(content)
            temporary.replace(path)
        finally:
            if temporary is not None:
                temporary.unlink(missing_ok=True)
    return [path for path, _ in prepared]


def extract_default_bitmap(executable: Path | None = None, root: Path = REPO) -> list[Path]:
    path = Path(executable) if executable is not None else (retail_exe() if root == REPO else root / 'build/orig/Meridian.exe')
    verified = read_verified(targets(root)['game'], path)
    pe = Pe(path)
    if pe.data != verified:
        raise InputError(f'{path}: input changed while reading')
    payloads = []
    for spec in _manifest(root)['default_bitmap']:
        rva, size = int(spec['rva'], 16), spec['count'] * spec['width']
        if not any(s['va'] <= rva and rva + size <= s['va'] + s['rsize'] for s in pe.sections):
            raise InputError(f"{spec['file']}: payload is not fully backed by PE file bytes")
        payloads.append((spec, pe.read(rva, size)))
    return _install(root, payloads)


def extract_sibling_assets(key: str, source: Path, root: Path = REPO) -> list[Path]:
    manifest = _manifest(root)['sibling_assets'][key]
    raw = Path(source).read_bytes()
    if hashlib.sha256(raw).hexdigest() != manifest['source_sha256']:
        raise InputError(f'{source}: sibling source differs from pinned SHA-256')
    text = raw.decode('latin1')
    payloads = []
    for spec in manifest['arrays']:
        pattern = (r'\b' + re.escape(spec['type']) + r'\s+' + re.escape(spec['name'])
                   + r'\[[^\]]*\][^=;]*=\s*\{([^}]*)\};')
        matches = re.findall(pattern, text)
        if len(matches) != 1:
            raise InputError(f"{source}: expected one initializer for {spec['name']}")
        # No evaluation of C++ source: admit only literal integer arrays.
        body = re.sub(r'/\*.*?\*/|//[^\n]*', '', matches[0], flags=re.S)
        values = []
        for token in body.split(','):
            token = token.strip()
            if not token:
                continue
            if re.fullmatch(r"'(?:[^'\\]|\\.)'", token):
                values.append(ord(ast.literal_eval(token)))
                continue
            if not re.fullmatch(r'0[xX][0-9a-fA-F]+|[0-9]+', token):
                raise InputError(f"{source}: nonliteral initializer in {spec['name']}")
            values.append(int(token, 16 if token.lower().startswith('0x') else 10))
        data = b''.join(value.to_bytes(spec['width'], 'little') for value in values)
        payloads.append((spec, data))
    return _install(root, payloads)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path)
    parser.add_argument('--sibling', choices=sorted(_manifest(REPO)['sibling_assets']))
    parser.add_argument('--source', type=Path)
    args = parser.parse_args(argv)
    if args.sibling and (args.source is None or args.exe is not None):
        parser.error('--sibling requires --source and cannot use --exe')
    if args.source is not None and args.sibling is None:
        parser.error('--source requires --sibling')
    try:
        paths = (extract_sibling_assets(args.sibling, args.source) if args.sibling
                 else extract_default_bitmap(args.exe))
    except (InputError, OSError, ValueError) as error:
        print(f'[assets] {error}', file=sys.stderr)
        return 1
    for path in paths:
        print(f'[assets] {path.relative_to(REPO)}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
