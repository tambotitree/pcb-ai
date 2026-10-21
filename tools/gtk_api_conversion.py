#!/usr/bin/env python3
"""Apply reviewed GTK migration rules and report APIs needing coordinated changes."""

import json
import re
from pathlib import Path

from c_conversion import NONCODE
from conversion import main

TABLE_PATH = Path(__file__).resolve().parents[1] / 'upgrade_data/gtk_api_migrations.json'
TABLE = json.loads(TABLE_PATH.read_text())
RULESET_VERSION = TABLE['ruleset_version']
RULES = TABLE['rules']


def transform(source, root, relative, tool):
  """Run the rule table on GTK C/header files, preserving comments and literals."""
  if not relative.startswith('src/hid/gtk/'):
    return source, 'Outside GTK directory; no rules selected.'
  text = source.decode()
  report = []
  for rule in RULES:
    if rule.get('skip_if_contains') and rule['skip_if_contains'] in text:
      continue
    if rule.get('path', relative) != relative:
      continue
    masked = NONCODE.sub(lambda match: ''.join('\n' if c == '\n' else ' '
                                              for c in match[0]), text)
    edits = []
    for match in re.finditer(rule['pattern'], masked):
      # A masked comment inside an expression must not be consumed by a rewrite.
      if text[match.start():match.end()] != match[0]:
        continue
      report.append({'rule': rule['id'], 'line': text.count('\n', 0, match.start()) + 1,
                     'status': 'rewrite' if rule['replacement'] is not None else 'needs_rule',
                     'reason': rule['reason'], 'target_api': rule['target_api'],
                     'compiler_suggestion': rule['compiler_suggestion'],
                     'gschem_examples': rule['gschem_examples']})
      if rule['replacement'] is not None:
        edits.append((match.start(), match.end(), rule['replacement']))
    for start, end, replacement in reversed(edits):
      text = text[:start] + replacement + text[end:]
  return text.encode(), json.dumps({'ruleset_version': RULESET_VERSION, 'findings': report})


if __name__ == '__main__':
  raise SystemExit(main('gtk-api', transform, 'python3'))
