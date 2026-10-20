#!/usr/bin/env python3
"""Exercise release contracts without bootstrapping the legacy build system."""

import io
import pathlib
import subprocess
import tarfile
import tempfile
import unittest

import version


class ReleaseTests(unittest.TestCase):
  """Check release identity and rejection of inconsistent release inputs."""

  def test_version(self):
    """Accept valid identities and reject invalid numbers and dates."""
    self.assertEqual(version.read_version('AC_INIT([pcb-ai], [0.1.0-20261020])'),
                     '0.1.0-20261020')
    for value in ('v0.1.0-20261020', '01.1.0-20261020', '0.1-20261020',
                  '0.1.0-20260230'):
      with self.assertRaises(ValueError):
        version.validate_version(value)

  def test_archive(self):
    """Accept a Git-free archive and reject an embedded version mismatch."""
    with tempfile.TemporaryDirectory() as directory:
      path = pathlib.Path(directory) / 'pcb-ai-0.1.0-20261020.tar.gz'
      for embedded in ('0.1.0-20261020', '0.2.0-20261020'):
        source = ('AC_INIT([pcb-ai], [' + embedded + '])').encode()
        with tarfile.open(path, 'w:gz') as archive:
          entry = tarfile.TarInfo('pcb-ai-0.1.0-20261020/configure.ac')
          entry.size = len(source)
          archive.addfile(entry, io.BytesIO(source))
        if embedded == '0.1.0-20261020':
          version.check_archive(path, embedded)
        else:
          with self.assertRaises(ValueError):
            version.check_archive(path, '0.1.0-20261020')

  def test_tag(self):
    """Accept an annotated HEAD tag, reject lightweight tags and dirty sources."""
    with tempfile.TemporaryDirectory() as directory:
      root = pathlib.Path(directory)
      def git(*arguments):
        """Run Git only in the disposable test repository."""
        return subprocess.run(['git', '-C', directory, *arguments], check=True,
                              capture_output=True)
      git('init')
      git('config', 'user.name', 'Release Test')
      git('config', 'user.email', 'test@example.invalid')
      (root / 'source').write_text('fixture')
      git('add', 'source')
      git('commit', '-m', 'fixture')
      git('tag', '-a', '0.1.0-20261020', '-m', 'fixture release')
      version.check_tag(root, '0.1.0-20261020')
      git('tag', '0.2.0-20261020')
      with self.assertRaises(ValueError):
        version.check_tag(root, '0.2.0-20261020')
      (root / 'source').write_text('dirty')
      with self.assertRaises(ValueError):
        version.check_tag(root, '0.1.0-20261020')


if __name__ == '__main__':
  unittest.main()
