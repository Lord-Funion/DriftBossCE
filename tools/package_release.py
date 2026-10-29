"""Create the Cemetech ZIP and single-transfer bundle from a full checkout."""
from pathlib import Path
import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
import zipfile

from verify_release import verify_ti, verify_sprites

ROOT = Path(__file__).resolve().parents[1]
VERSION = '0.2'

def copy(source, destination):
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'releases')
    parser.add_argument('--convbin', default=shutil.which('convbin') or 'convbin')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    files = sorted(p for p in (ROOT / 'dist').iterdir() if p.suffix in ('.8xp', '.8xv'))
    if len(files) != 8:
        raise ValueError('Expected eight calculator transfer files')
    manifest = [verify_ti(p) for p in files]
    verify_sprites()
    bundle = out / f'DriftBossCE-v{VERSION}-all-in-one.b84'
    command = [args.convbin, '-j', 'bin', '-k', 'b84', '-o', str(bundle)]
    for f in files:
        command.extend(['-i', str(f)])
    subprocess.run(command, check=True)
    with zipfile.ZipFile(bundle) as z:
        if z.testzip() is not None:
            raise ValueError('Bundle CRC failure')
        members = [n for n in z.namelist() if n not in ('METADATA', '_CHECKSUM')]
        if len(members) != 8 or '84CE' not in z.read('METADATA').decode():
            raise ValueError('Unexpected bundle metadata or member count')
        for n in members:
            if z.read(n) != (ROOT / 'dist' / Path(n).name).read_bytes():
                raise ValueError(f'Bundle member differs: {n}')
    archive = out / f'DriftBossCE-v{VERSION}-Cemetech.zip'
    with tempfile.TemporaryDirectory(prefix='driftboss-package-') as temporary:
        package = Path(temporary) / f'DriftBossCE-v{VERSION}'
        package.mkdir()
        for f in files:
            copy(f, package / 'Calculator' / f.name)
        copy(bundle, package / bundle.name)
        for name in ('README.txt', 'LICENSE.md', 'CREDITS.md', 'CHANGELOG.md'):
            copy(ROOT / name, package / name)
        for f in (ROOT / 'licenses').iterdir():
            if f.is_file():
                copy(f, package / 'Licenses' / f.name)
        for f in (ROOT / 'docs').iterdir():
            if f.is_file():
                copy(f, package / 'Docs' / f.name)
        for f in (ROOT / 'cemetech').iterdir():
            if f.is_file():
                copy(f, package / 'Cemetech' / f.name)
        for f in (ROOT / 'cemetech/screenshots').glob('*.gif'):
            copy(f, package / 'Screenshots/Cemetech' / f.name)
        for f in (ROOT / 'docs/media').glob('*.png'):
            copy(f, package / 'Screenshots' / f.name)
        copy(ROOT / 'docs/media/gameplay.gif', package / 'Screenshots/gameplay-full.gif')
        with (package / 'Docs/MEDIA.md').open('a', encoding='utf8') as f:
            f.write('\n## Release ZIP media\n\nThe ZIP includes `Screenshots/gameplay-full.gif`, full-size PNGs, and the four compact listing GIFs under `Screenshots/Cemetech/`. The second full-size 8-second GIF and the SVG banner remain available in the GitHub repository.\n')
        for directory in ('src', 'assets', 'dist', 'tests', 'licenses'):
            for f in (ROOT / directory).rglob('*'):
                if f.is_file():
                    copy(f, package / 'Source' / f.relative_to(ROOT))
        for name in ('makefile', 'icon.png', 'LICENSE.md', 'CREDITS.md'):
            copy(ROOT / name, package / 'Source' / name)
        for name in ('build_assets.py', 'verify_release.py'):
            copy(ROOT / 'tools' / name, package / 'Source/tools' / name)
        for f in (ROOT / 'tools/capture').iterdir():
            if f.is_file():
                copy(f, package / 'Source/tools/capture' / f.name)
        copy(ROOT / 'docs/BUILD.md', package / 'Source/BUILD.md')
        (package / 'Source/README.md').write_text('# Drift Boss CE v0.2 source\n\nRun `make` here with CE toolchain v15.0 on PATH. The generated artwork banks and tested calculator files are included. Run `python tools/build_assets.py` to rebuild the AppVars and `python tools/verify_release.py --built bin/DRIFTB.8xp` to check your build. See [the build guide](../Docs/BUILD.md) and [installation guide](../Docs/INSTALL.md). Media/release tooling is available in the full GitHub checkout.\n', encoding='utf8')
        (package / 'checksums.json').write_text(json.dumps(manifest, indent=2) + '\n')
        with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
            for f in sorted(package.rglob('*')):
                if f.is_file():
                    info = zipfile.ZipInfo(f.relative_to(package.parent).as_posix(), date_time=(2026, 10, 29, 12, 0, 0))
                    info.compress_type = zipfile.ZIP_DEFLATED
                    info.external_attr = 0o100644 << 16
                    z.writestr(info, f.read_bytes())
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None:
            raise ValueError('ZIP CRC failure')
        prefix = f'DriftBossCE-v{VERSION}/'
        for f in files:
            if z.read(prefix + 'Calculator/' + f.name) != f.read_bytes():
                raise ValueError(f'ZIP member differs: {f.name}')
        required = ['README.txt', 'Source/src/game.c', 'Source/src/render.c', 'Source/assets/DBAS0.bin', 'Cemetech/description.txt', 'Screenshots/gameplay-full.gif']
        for name in required:
            if prefix + name not in z.namelist():
                raise ValueError(f'Missing release member: {name}')
    if archive.stat().st_size >= 2_000_000:
        raise ValueError('Cemetech archive exceeds conservative 2 MB budget')
    checksums = ''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in (archive, bundle))
    (out / 'SHA256SUMS.txt').write_text(checksums)
    print(f'ZIP: {archive} ({archive.stat().st_size:,} bytes)')
    print(f'Bundle: {bundle} ({bundle.stat().st_size:,} bytes)')
    print('PASS bundle membership, ZIP membership, CRCs, and size budget')

if __name__ == '__main__':
    main()
