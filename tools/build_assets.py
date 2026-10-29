"""Recreate the archived graphics AppVars from the committed data."""
from pathlib import Path
import argparse
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--convbin', default=shutil.which('convbin') or 'convbin')
    parser.add_argument('--out', type=Path, default=ROOT / 'bin')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    for i in range(3):
        name = f'DBAS{i}'
        subprocess.run([args.convbin, '-j', 'bin', '-k', '8xv', '-i', str(ROOT / 'assets' / f'{name}.bin'), '-o', str(args.out / f'{name}.8xv'), '-n', name, '-r'], check=True)
    print(f'Created DBAS0, DBAS1, DBAS2 in {args.out}')

if __name__ == '__main__':
    main()
