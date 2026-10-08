"""Export one standalone C++ source project from a committed reconstruction."""

import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import tarfile
import tempfile
import tomllib

from hobbit.clean.lexer import clean_source

TEMPLATE = 'scripts/hobbit/clean/project/'
MARKER = '.hobbit-source-export'
BRIDGES = ('__init__.py', 'wine.py', 'cl.py', 'link.py', 'rc.py')


def snapshot(repo, revision, working=False):
    resolved = subprocess.run(
        ['git', '-C', str(repo), 'rev-parse', '--verify', f'{revision}^{{commit}}'],
        text=True, capture_output=True)
    if resolved.returncode and not (working and revision == 'HEAD'):
        raise ValueError(f'no committed revision {revision!r}; use --working-tree for a new checkout')
    commit = resolved.stdout.strip() if not resolved.returncode else '(unborn working tree)'
    if working:
        names = subprocess.check_output(['git', '-C', str(repo), 'ls-files', '--cached', '--others', '--exclude-standard', '-z'],
                                        text=True).split('\0')
        return commit, {name: (repo / name).read_bytes() for name in names
                        if name and not (repo / name).is_symlink() and (repo / name).is_file()}
    data = subprocess.check_output(['git', '-C', str(repo), 'archive', commit])
    with tarfile.open(fileobj=io.BytesIO(data)) as archive:
        return commit, {entry.name: archive.extractfile(entry).read()
                        for entry in archive if entry.isfile()}


def generate(files):
    manifest = tomllib.loads(files['config/units.toml'].decode())
    units = []
    names = set()
    for unit in manifest['unit']:
        name, source = unit['unit'], unit['source']
        if name in names or not name or any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-' for c in name):
            raise ValueError(f'invalid or duplicate unit name: {name}')
        names.add(name)
        if not source.startswith(('src/', 'vendor/')):
            raise ValueError(f'unsupported unit source: {source}')
        units.append({'name': name, 'source': source, 'flags': manifest['flags'][unit['flags']]})
    if not units:
        raise ValueError('empty source manifest')
    sources = {unit['source'] for unit in units}
    sources.update(name for name in files if name.startswith(('src/', 'include/'))
                   and Path(name).suffix.lower() in ('.h', '.hpp', '.inl', '.inc', '.rc'))
    sources.discard('include/rva.h')
    output = {}
    for name in sorted(sources):
        try:
            data = files[name]
            output[name] = (data if name.startswith('vendor/') else
                            clean_source(data.decode('utf-8')).encode('utf-8'))
        except (ValueError, KeyError) as error:
            raise ValueError(f'{name}: {error}') from error
    for name, data in sorted(files.items()):
        if name.startswith(TEMPLATE):
            output[name.removeprefix(TEMPLATE)] = data
        elif name == 'src/Apps/Meridian/res/README.md':
            output[name] = data
        elif name.startswith('vendor/') and (Path(name).suffix.lower() in ('.h', '.hpp', '.inl', '.inc')
                                            or Path(name).name in ('README', 'LICENSE', 'COPYING')):
            output[name] = data
    for name in BRIDGES:
        output[f'scripts/hobbitbuild/tool/{name}'] = files[f'scripts/hobbit/tool/{name}'].replace(
            b'hobbit.', b'hobbitbuild.').replace(b'`hobbit init`', b'`python3 build.py`')
    for name in ('core/inputs.py', 'core/pe.py', 'core/assets.py',
                 'rsrc/icon.py', 'rsrc/res.py'):
        output[f'scripts/hobbitbuild/{name}'] = files[f'scripts/hobbit/{name}'].replace(
            b'hobbit.', b'hobbitbuild.')
    output['scripts/hobbitbuild/rsrc/__init__.py'] = b''
    for name in ('targets.json', 'local-assets.json'):
        output[f'config/retail/{name}'] = files[f'config/retail/{name}']
    for name in ('scripts/hobbitbuild/__init__.py', 'scripts/hobbitbuild/core/__init__.py'):
        output[name] = b''
    for name in ('LICENSE', 'NOTICE.md', 'nix/toolchain.nix'):
        if name in files:
            output[name] = files[name]
    output['build.json'] = (json.dumps({
        'units': units,
        'resource': 'src/Apps/Meridian/Meridian.rc',
        'link': None,
    }, indent=2) + '\n').encode()
    if 'flake.lock' in files:
        lock = json.loads(files['flake.lock'])
        node = lock['nodes'][lock['nodes']['root']['inputs']['nixpkgs']]
        output['flake.lock'] = (json.dumps({'nodes': {'nixpkgs': node,
            'root': {'inputs': {'nixpkgs': 'nixpkgs'}}},
            'root': 'root', 'version': lock['version']}, indent=2) + '\n').encode()
    for name in output:
        path = PurePosixPath(name)
        if path.is_absolute() or any(part in ('..', '.git', 'tests', '__pycache__') for part in path.parts):
            raise ValueError(f'unsafe output name: {name}')
    for name in ('build.py', 'flake.nix', 'scripts/hobbitbuild/core/paths.py'):
        if name not in output:
            raise ValueError(f'missing export template: {name}')
    return output


def validate_output(repo, requested):
    path = requested.absolute()
    if path.is_symlink() or any(parent.is_symlink() for parent in path.parents):
        raise ValueError('output must not traverse symlinks')
    path, repo = path.resolve(), repo.resolve()
    if path == repo or path in repo.parents:
        raise ValueError('output must not contain the repository')
    if path.is_relative_to(repo) and (not path.is_relative_to(repo / 'build') or path == repo / 'build'):
        raise ValueError('output in this checkout must be a child of build/')
    if path.exists():
        marker = path / MARKER
        if not marker.is_file() or marker.is_symlink():
            raise ValueError('existing output is not a generated directory')
        if any(path.rglob('.git')):
            raise ValueError('output contains a Git repository; export to a fresh directory')
        try:
            metadata = json.loads(marker.read_text())
            if not isinstance(metadata, dict) or metadata.get('generator') != 'hobbit clean':
                raise ValueError('unrecognized output marker')
        except json.JSONDecodeError as error:
            raise ValueError('invalid output marker') from error
    return path


def write_output(repo, requested, files, commit, working=False):
    output = validate_output(repo, requested)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.hobbit-source-', dir=output.parent) as temp:
        staging = Path(temp) / 'project'
        staging.mkdir()
        for name, data in sorted(files.items()):
            path = staging / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        (staging / MARKER).write_text(json.dumps({
            'generator': 'hobbit clean', 'commit': commit, 'working_tree': working}) + '\n')
        if output.exists():
            shutil.rmtree(output)
        staging.rename(output)
    return output


def main(argv=None):
    from hobbit.core.paths import REPO
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=REPO / 'build/clean-source')
    parser.add_argument('--ref', default='HEAD')
    parser.add_argument('--working-tree', action='store_true', help='preview working files, including untracked files not ignored by Git')
    parser.add_argument('--verify', action='store_true', help='compile exported units and resources in its own Nix shell')
    args = parser.parse_args(argv)
    try:
        if args.working_tree and args.ref != 'HEAD':
            raise ValueError('--working-tree cannot be combined with --ref')
        commit, inputs = snapshot(REPO, args.ref, args.working_tree)
        files = generate(inputs)
        output = write_output(REPO, args.out, files, commit, args.working_tree)
        digest = hashlib.sha256(b''.join(name.encode() + b'\0' + data
                                         for name, data in sorted(files.items()))).hexdigest()
        print(f'Exported {len(files)} files to {output}\nSource: {commit}\nSHA-256: {digest}', flush=True)
        if args.verify:
            from hobbit.core.paths import retail_exe
            subprocess.run(['nix', 'develop', 'path:.', '-c', 'python3', 'build.py',
                            '--exe', str(retail_exe().resolve())],
                           cwd=output, check=True)
        return 0
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.error(str(error))
