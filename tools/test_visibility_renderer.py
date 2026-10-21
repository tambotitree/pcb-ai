#!/usr/bin/env python3
"""Compile and exercise the actual visibility renderer independently of the GUI."""

import argparse
import os
import pathlib
import shlex
import subprocess


def main():
  """Build the fixture against GTK3; report display-unavailable as a skip."""
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--source', type=pathlib.Path, help='staged source to validate before applying')
  args = parser.parse_args()
  root = pathlib.Path(__file__).resolve().parents[1]
  work = root / 'upgrade_data/scratch/visibility-renderer-test'
  work.mkdir(parents=True, exist_ok=True)
  binary = work / 'test-visibility-renderer'
  source = args.source or root / 'src/hid/gtk/ghid-cell-renderer-visibility.c'
  flags = shlex.split(subprocess.check_output(
    ['pkg-config', '--cflags', '--libs', 'gtk+-3.0'], text=True))
  includes = [root, root / 'src', root / 'src/hid/gtk',
              root / 'upgrade_data/scratch/build-gtk3']
  command = ['cc', '-std=gnu23', '-g3', '-O0', '-DHAVE_CONFIG_H',
             '-Wall', '-Wextra', '-Werror', '-Wpedantic', '-Wstrict-prototypes',
             '-Wmissing-prototypes', *['-I' + str(path) for path in includes],
             str(source.resolve()), str(root / 'tools/test_visibility_renderer.c'),
             *flags, '-o', str(binary)]
  subprocess.run(command, check=True)
  result = subprocess.run([str(binary), str(work)],
                          env={**os.environ, 'GTK_THEME': 'Adwaita'})
  if result.returncode == 77:
    print('SKIP: GTK display unavailable; compilation passed')
    return 77
  result.check_returncode()
  print('PASS: strict compile, active/inactive/prelit pixels, clipping, Cairo ownership/state,')
  print('color fallback, toggle signal and object finalization')
  return 0


if __name__ == '__main__':
  raise SystemExit(main())
