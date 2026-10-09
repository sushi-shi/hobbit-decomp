#!/usr/bin/env python3
"""Install the complete VC6 SP5 tree header from preserved local media.

Chained-cabinet extraction adapted from HoMM1 Buka scripts/homm1/toolchain.py
extract_media_files at ea77041d959fd3f449561564fa3f672114da41a7. Only the
required original header is selected. No payload or acquisition URL is saved
in public source; the destination is ignored build/.
"""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024*1024), b''):
            value.update(chunk)
    return value.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--media', required=True, type=Path)
    parser.add_argument('--pins', type=Path, default=Path('nix/toolchain-identities.json'))
    parser.add_argument('--out-dir', type=Path)
    args = parser.parse_args()
    pin = json.loads(args.pins.read_text())['standard_headers']['XTREE']
    if digest(args.media) != pin['media_sha256']:
        raise SystemExit('SP5 media SHA-256 differs from the pin')
    output = args.out_dir or Path(pin['local_path']).parent
    output.mkdir(parents=True, exist_ok=True)
    sevenzip = shutil.which('7z') or shutil.which('7zz')
    cabinet = shutil.which('cabextract')
    if not sevenzip or not cabinet:
        raise SystemExit('7z and cabextract are required; enter the Hobbit tool shell')
    with tempfile.TemporaryDirectory(prefix='.sp5-', dir=output) as temporary:
        scratch = Path(temporary)
        cabs = scratch/'cabs'
        extracted = scratch/'extracted'
        subprocess.run([sevenzip,'x','-y',f'-o{cabs}',str(args.media.resolve()),'VS6sp5*.cab'],check=True,stdout=subprocess.DEVNULL)
        subprocess.run([cabinet,'-q','-F',pin['media_path'],'-d',str(extracted),str(cabs/'VS6sp51.cab')],check=True)
        header = extracted/pin['media_path']
        if digest(header) != pin['sha256']:
            raise SystemExit('extracted complete XTREE SHA-256 differs from the pin')
        destination = output/'XTREE'
        if not destination.exists() or destination.read_bytes() != header.read_bytes():
            staged = output/'.XTREE.install'
            shutil.copyfile(header,staged)
            staged.replace(destination)
    print(f'Installed complete SP5 XTREE: {destination} SHA-256 {pin["sha256"]}')


if __name__ == '__main__':
    main()
