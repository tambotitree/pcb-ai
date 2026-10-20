#!/usr/bin/env python3
"""Run inventory-selected mechanical converters in a disposable staging tree."""

import argparse
import datetime
import difflib
import hashlib
import json
import pathlib
import shutil
import subprocess

from upgrade_inventory import write_inventory


def main(kind, transform, default_tool):
  """Preview by default; apply only selected, unchanged inventory inputs on request."""
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--apply', action='store_true', help='write candidates with scratch backups')
  parser.add_argument('--path', action='append', help='select an inventoried path; repeatable')
  parser.add_argument('--tool', help='override converter executable')
  args = parser.parse_args()
  root = pathlib.Path(__file__).resolve().parents[1]
  inventory = json.loads((root / 'upgrade_data/source_inventory.json').read_text())
  languages = {'python': ('Python',), 'autoconf': ('Autoconf',),
               'c': ('C', 'C header'), 'c-signatures': ('C',), 'scheme': ('Scheme',)}[kind]
  entries = [entry for entry in inventory['files'] if entry['language'] in languages]
  if args.path:
    unknown = set(args.path) - {entry['path'] for entry in entries}
    if unknown:
      parser.error('paths absent from selected inventory type: ' + ', '.join(sorted(unknown)))
    entries = [entry for entry in entries if entry['path'] in args.path]
  executable = args.tool or default_tool
  tool = shutil.which(executable)
  if not tool:
    parser.error('converter not found: ' + executable)
  version_command = ([shutil.which('python3.12') or 'python3', '--version']
                     if kind == 'python' else [tool, '--version'])
  tool_version = subprocess.run(version_command, capture_output=True, text=True)
  tool_identity = (tool_version.stdout + tool_version.stderr).strip()
  tool_hash = hashlib.sha256(pathlib.Path(tool).read_bytes()).hexdigest()
  stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
  report_dir = root / 'upgrade_data/scratch' / (kind + '-' + stamp)
  report_dir.mkdir(parents=True)
  records = []
  candidates = []
  for entry in entries:
    relative = entry['path']
    path = root / relative
    record = {'path': relative, 'status': 'error'}
    try:
      if not path.resolve().is_relative_to(root) or path.is_symlink():
        raise ValueError('input must be a regular file within the checkout')
      source = path.read_bytes()
      if hashlib.sha256(source).hexdigest() != entry['sha256']:
        raise ValueError('source changed since inventory; regenerate inventory first')
      candidate, diagnostics = transform(source, root, relative, tool)
      record['diagnostics'] = diagnostics
      record['status'] = 'unchanged' if candidate == source else 'proposed'
      if candidate != source:
        diff = ''.join(difflib.unified_diff(
          source.decode().splitlines(True), candidate.decode().splitlines(True),
          fromfile='a/' + relative, tofile='b/' + relative))
        print(diff, end='')
        record['diff'] = diff
        candidates.append((path, relative, source, candidate, record))
    except (ValueError, OSError, SyntaxError) as error:
      record['error'] = str(error)
    records.append(record)
  failed = any(record['status'] == 'error' for record in records)
  # Finish all conversion/validation work before writing any source files.
  if args.apply and not failed:
    for path, relative, source, candidate, record in candidates:
      if path.read_bytes() != source:
        raise ValueError('source changed during conversion: ' + relative)
    for path, relative, source, candidate, record in candidates:
      backup = report_dir / 'originals' / relative
      backup.parent.mkdir(parents=True, exist_ok=True)
      backup.write_bytes(source)
      path.write_bytes(candidate)
      record['status'] = 'applied'
  if args.apply and not failed:
    by_path = {entry['path']: entry for entry in inventory['files']}
    for path, relative, source, candidate, record in candidates:
      entry = by_path[relative]
      new_hash = hashlib.sha256(candidate).hexdigest()
      entry['versions']['conversions'].append({
        'converter': kind, 'tool': tool, 'tool_version_output': tool_identity, 'tool_sha256': tool_hash,
        'project_version': inventory['project_version'], 'timestamp_utc': stamp,
        'before_sha256': entry['sha256'], 'after_sha256': new_hash,
        'status': 'applied_unverified',
      })
      entry['sha256'] = new_hash
      entry['lines'] = len(candidate.splitlines())
      entry['clues'] = {}  # Regenerate the inventory for fresh line numbers.
      entry['review']['status'] = 'needs_review'
    write_inventory(root / 'upgrade_data/source_inventory.json', inventory)
  report = {'converter': kind, 'tool': tool, 'tool_version_output': tool_identity, 'tool_sha256': tool_hash, 'apply_requested': args.apply,
            'files': records, 'failed': failed}
  (report_dir / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
  for record in records:
    print(record['status'] + ': ' + record['path'])
    if 'error' in record:
      print(record['error'])
  print('Report: ' + str(report_dir / 'report.json'))
  return int(failed)
