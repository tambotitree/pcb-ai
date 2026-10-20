#!/usr/bin/env python3
"""Preview or apply Autoconf macro updates selected by the source inventory."""

import pathlib
import shutil
import subprocess
import tempfile

from conversion import main


def transform(source, root, relative, tool):
  """Update Autoconf macros in staging with a copy of local macro context."""
  with tempfile.TemporaryDirectory(prefix='pcb-autoconf-') as directory:
    stage = pathlib.Path(directory)
    target = stage / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(source)
    if (root / 'm4').is_dir():
      shutil.copytree(root / 'm4', stage / 'm4')
    if (root / 'aclocal.m4').is_file():
      shutil.copy2(root / 'aclocal.m4', stage / 'aclocal.m4')
    result = subprocess.run([tool, '-I', str(stage / 'm4'), str(target)],
                            cwd=stage, capture_output=True, text=True)
    diagnostics = result.stdout + result.stderr
    if result.returncode:
      raise ValueError(diagnostics.strip())
    return target.read_bytes(), diagnostics

if __name__ == '__main__':
  raise SystemExit(main('autoconf', transform, 'autoupdate'))
