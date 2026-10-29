"""Validate the TI files, sprite banks, and optional program rebuild."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import struct

ROOT = Path(__file__).resolve().parents[1]

def check(condition, message):
    if not condition:
        raise ValueError(message)

def verify_ti(path):
    b = path.read_bytes()
    check(b[:8] == b'**TI83F*', f'{path}: signature')
    check(len(b) == 57 + struct.unpack_from('<H', b, 53)[0], f'{path}: length')
    check(sum(b[55:-2]) & 65535 == struct.unpack_from('<H', b, len(b)-2)[0], f'{path}: checksum')
    check(struct.unpack_from('<H', b, 55)[0] == 13, f'{path}: variable header')
    check(b[69] == 128, f'{path}: archive flag')
    check(b[59] == (6 if path.suffix.lower() == '.8xp' else 21), f'{path}: type')
    check(struct.unpack_from('<H', b, 57)[0] == struct.unpack_from('<H', b, 70)[0], f'{path}: size copies')
    name = b[60:68].split(b'\0')[0].decode('ascii')
    check(name.casefold() == path.stem.casefold(), f'{path}: variable name')
    if name.startswith('DBAS'):
        check(b[74:-2] == (ROOT / 'assets' / f'{name}.bin').read_bytes(), f'{path}: bank differs')
    return {'file': path.name, 'variable': name, 'bytes': len(b), 'sha256': hashlib.sha256(b).hexdigest()}

def verify_sprites():
    source = (ROOT / 'src/assets.c').read_text()
    refs = re.search(r'sprite_refs\[SPRITE_COUNT\] = \{(.*?)\};', source, re.S)[1]
    refs = [tuple(map(int, r)) for r in re.findall(r'\{\s*(\d+),\s*(\d+),\s*(-?\d+),\s*(-?\d+)\s*\}', refs)]
    check(len(refs) == 133, 'sprite reference count')
    banks = [(ROOT / f'assets/DBAS{i}.bin').read_bytes() for i in range(3)]
    sizes = list(map(int, re.search(r'asset_bank_sizes\[ASSET_BANKS\] = \{([^}]+)', source)[1].split(',')))
    check(list(map(len, banks)) == sizes, 'asset bank sizes')
    for b in banks:
        check(b[:8] == b'DBCEAS1\0', 'asset bank signature')
    for index, (offset, bank, _ox, _oy) in enumerate(refs):
        b = banks[bank]
        w, h = b[offset:offset+2]
        p = offset + 2
        check(w > 0 and h > 0, f'sprite {index}: dimensions')
        for y in range(h):
            previous = 0
            while True:
                check(p < len(b), f'sprite {index}: truncated row')
                start = b[p]
                p += 1
                if start == 255:
                    break
                check(p < len(b), f'sprite {index}: missing length')
                n = b[p]
                p += 1
                check(n > 0 and start >= previous and start + n <= w and p + n <= len(b), f'sprite {index}: invalid run')
                p += n
                previous = start + n
        check(p-offset <= 49152, f'sprite {index}: larger than cache arena')
    return len(refs)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--built', type=Path)
    parser.add_argument('--write-manifest', action='store_true')
    args = parser.parse_args()
    paths = sorted(p for p in (ROOT / 'dist').iterdir() if p.suffix.lower() in ('.8xp', '.8xv'))
    check(len(paths) == 8, 'expected exactly eight transfer files')
    report = [verify_ti(p) for p in paths]
    count = verify_sprites()
    if args.built:
        verify_ti(args.built)
        check(args.built.read_bytes()[55:] == (ROOT / 'dist/DRIFTB.8xp').read_bytes()[55:], 'rebuilt TI variable payload differs from tested release')
        print('PASS rebuilt program matches the tested release variable payload')
    manifest = ROOT / 'dist/checksums.json'
    if args.write_manifest:
        manifest.write_text(json.dumps(report, indent=2) + '\n')
    elif manifest.exists():
        check(json.loads(manifest.read_text()) == report, 'release manifest differs')
    print(f"PASS {len(report)} archived TI files, three exact banks, {count} sprite streams; {sum(r['bytes'] for r in report):,} transfer-file bytes")

if __name__ == '__main__':
    main()
