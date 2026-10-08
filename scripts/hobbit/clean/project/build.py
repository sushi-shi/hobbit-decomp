"""Compile the exported reconstruction units and resources; never launch a game."""

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'scripts'))

from hobbitbuild.tool import ToolError, cl, link, rc
from hobbitbuild.tool.wine import init_prefix, shutdown_wineserver, verify_prefix, winepath
from hobbitbuild.core.assets import extract_default_bitmap
from hobbitbuild.rsrc.icon import stage_script, verified_resources

def shared_fingerprint():
    """Conservative header closure: a changed header invalidates every object."""
    digest = hashlib.sha256()
    paths = [ROOT / 'build.py', ROOT / 'build.json',
             ROOT / 'config/retail/targets.json', ROOT / 'config/retail/local-assets.json']
    for directory in ('src', 'include', 'vendor', 'scripts', 'build/gen/retail-assets'):
        paths.extend(path for path in (ROOT / directory).rglob('*')
                     if path.suffix.lower() in ('.h', '.hpp', '.inl', '.inc', '.py'))
    for path in sorted(paths):
        digest.update(str(path.relative_to(ROOT)).encode() + b'\0' + path.read_bytes())
    for variable in ('MSVC_DIR', 'DXSDK_DIR'):
        digest.update(os.environ.get(variable, '').encode() + b'\0')
    return digest.digest()


def compile_unit(unit, directory, shared):
    source = ROOT / unit['source']
    obj = directory / (unit['name'] + '.obj')
    stamp = obj.with_suffix('.sha256')
    expected = hashlib.sha256(shared + source.read_bytes()
                              + json.dumps(unit, sort_keys=True).encode()).hexdigest()
    if obj.is_file() and stamp.is_file() and stamp.read_text() == expected:
        return obj, False
    stamp.unlink(missing_ok=True)
    cl.compile(source, obj, unit['flags'])
    stamp.write_text(expected)
    return obj, True


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--exe', type=Path, help='locally supplied pinned Meridian.exe (or HOBBIT_EXE)')
    parser.add_argument('--link', action='store_true', help='link only with an explicit build.json link profile')
    args = parser.parse_args(argv)
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    os.chdir(ROOT)
    # Always isolate the project from the caller's matching/build Wine prefix.
    os.environ['WINEPREFIX'] = str(ROOT / 'build/wineprefix')
    os.environ.setdefault('WINEDLLOVERRIDES', 'mscoree,mshtml=')
    build = ROOT / 'build'
    objects = build / 'obj'
    libraries = build / 'lib'
    objects.mkdir(parents=True, exist_ok=True)
    libraries.mkdir(parents=True, exist_ok=True)
    try:
        config = json.loads((ROOT / 'build.json').read_text())
        if args.link and not config.get('link'):
            raise ToolError('full executable link unavailable: reconstruction is incomplete; '
                            'build.json needs an evidence-backed link profile and complete sources')
        extract_default_bitmap(args.exe, root=ROOT)
        resource_script = None
        if config.get('resource'):
            resource_script = stage_script(ROOT / config['resource'], build / 'rsrc',
                                           verified_resources(args.exe))
        init_prefix()
        verify_prefix()
        units = config['units']
        shared = shared_fingerprint()
        compiled = 0
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            jobs = [pool.submit(compile_unit, unit, objects, shared) for unit in units]
            for count, future in enumerate(as_completed(jobs), 1):
                obj, changed = future.result()
                compiled += changed
                if changed:
                    print(f'[{count}/{len(units)}] {obj.stem}', flush=True)
        print(f'Compiled {compiled} of {len(units)} units', flush=True)
        resource = build / 'Meridian.res'
        if config.get('resource'):
            rc.compile(resource_script, resource)
            print(resource)
        if args.link:
            profile = config['link']
            executable = build / profile['output']
            arguments = [*profile['flags'], f'/OUT:{winepath(executable)}']
            arguments += [winepath(objects / (unit['name'] + '.obj')) for unit in units]
            if config.get('resource'):
                arguments.append(winepath(resource))
            arguments += profile['libraries']
            output = link.link(arguments, cwd=build, expect=[executable])
            (build / 'link.log').write_text(output)
            print(executable)
        else:
            print('Source subset compiled. Full executable reconstruction is incomplete.')
        return 0
    except (ToolError, OSError, KeyError, ValueError) as error:
        print(error, file=sys.stderr)
        return 1
    finally:
        shutdown_wineserver()


if __name__ == '__main__':
    raise SystemExit(main())
