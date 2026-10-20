#!/usr/bin/env python3
"""Keep PCB-ai's configured version, release tag and archive identity aligned."""

import argparse
import datetime
import pathlib
import re
import subprocess
import sys
import tarfile

INIT = re.compile(r"AC_INIT\(\[pcb-ai\],\s*\[([^\]]+)\]")


def validate_version(version):
  """Reject malformed release numbers and impossible calendar dates."""
  if not re.fullmatch(r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\."
                      r"(0|[1-9][0-9]*)-[0-9]{8}", version):
    raise ValueError("expected MAJOR.MINOR.PATCH-YYYYMMDD")
  datetime.datetime.strptime(version.rsplit("-", 1)[1], "%Y%m%d")
  return version


def read_version(source):
  """Read the single authoritative literal from configure.ac contents."""
  matches = INIT.findall(source)
  if len(matches) != 1:
    raise ValueError("expected exactly one literal pcb-ai AC_INIT version")
  return validate_version(matches[0])


def git_output(root, *arguments):
  """Run a read-only Git command in the selected source tree."""
  return subprocess.check_output(
    ["git", "-C", str(root), *arguments], text=True,
    stderr=subprocess.PIPE).strip()


def check_tag(root, version):
  """Require a clean checkout and an annotated matching tag at HEAD."""
  if git_output(root, "status", "--porcelain", "--untracked-files=normal"):
    raise ValueError("release checkout is dirty; commit or remove build outputs first")
  reference = "refs/tags/" + version
  if git_output(root, "cat-file", "-t", reference) != "tag":
    raise ValueError("release tag must be annotated")
  if git_output(root, "rev-parse", reference + "^{commit}") != git_output(root, "rev-parse", "HEAD"):
    raise ValueError("release tag does not point at HEAD")


def check_archive(path, version):
  """Verify archive basename, sole top directory and embedded source version."""
  stem = "pcb-ai-" + version
  if path.name not in (stem + ".tar.gz", stem + ".tar.bz2", stem + ".tar.xz"):
    raise ValueError("archive filename must match " + stem + ".tar.{gz,bz2,xz}")
  with tarfile.open(path, "r:*") as archive:
    names = [member.name.removeprefix("./") for member in archive.getmembers()]
    if not names or any(name.split("/")[0] != stem for name in names):
      raise ValueError("archive must contain only the matching package directory")
    members = [member for member in archive.getmembers()
               if member.name.removeprefix("./") == stem + "/configure.ac"]
    if len(members) != 1 or not members[0].isfile():
      raise ValueError("archive needs exactly one regular configure.ac")
    with archive.extractfile(members[0]) as source:
      if read_version(source.read().decode("utf-8")) != version:
        raise ValueError("archive configure.ac version does not match")


def main():
  """Show, update or validate release identity; never create tags or push."""
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument("command", choices=("show", "set", "check"))
  parser.add_argument("version", nargs="?")
  parser.add_argument("--tag", action="store_true", help="require annotated release tag at HEAD")
  parser.add_argument("--archive", type=pathlib.Path, help="validate a make dist archive")
  args = parser.parse_args()
  if args.command != "set" and args.version:
    parser.error("version is only accepted by set")
  if args.command != "check" and (args.tag or args.archive):
    parser.error("--tag and --archive require check")
  root = pathlib.Path(__file__).resolve().parents[1]
  path = root / "configure.ac"
  source = path.read_text()
  version = read_version(source)
  if args.command == "set":
    if not args.version:
      parser.error("set requires a version")
    version = validate_version(args.version)
    path.write_text(INIT.sub(lambda match: match[0].replace(match[1], version), source))
  if args.command == "check":
    if args.tag:
      check_tag(root, version)
    if args.archive:
      check_archive(args.archive, version)
  print(version)
  print("Tag: " + version)
  print("Tarball: pcb-ai-" + version + ".tar.gz")
  return 0


if __name__ == "__main__":
  try:
    sys.exit(main())
  except (ValueError, OSError, subprocess.CalledProcessError, tarfile.TarError) as error:
    print("version: " + str(error), file=sys.stderr)
    sys.exit(1)
