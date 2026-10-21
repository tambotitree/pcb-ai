#!/usr/bin/env python3
"""Inventory code uses of -1; classify review candidates without changing semantics."""

import collections
import json
import re
from pathlib import Path

from c_conversion import NONCODE


def classify(line, before):
    """Offer coarse review categories, never an automatic defect verdict."""
    if re.search(r'\(\s*(?:void|\w+)\s*\*\s*\).*?-\s*1', line):
        return 'pointer-sentinel-review'
    if re.search(r'\breturn\s*-\s*1', line): return 'negative-result-contract-review'
    if re.search(r'\b(?:last\w*|\w*(?:state|status|mode|cache)\w*)\s*=\s*-\s*1', line, re.I):
        return 'state-or-cache-review'
    if re.search(r'\b(?:\w*(?:index|layer|group|offset|found|idx)\w*)\s*=\s*-\s*1', line, re.I):
        return 'index-or-not-found-review'
    if before and (before[-1].isalnum() or before[-1] in ')]'):
        return 'arithmetic-or-sentinel-review'
    return 'negative-constant-review'


def main():
    """Read inventoried C sources and write a reproducible, versioned candidate set."""
    root = Path(__file__).resolve().parents[1]
    inventory = json.loads((root / 'upgrade_data/source_inventory.json').read_text())
    rows = []
    for entry in inventory['files']:
        if entry['language'] not in ('C', 'C header', 'C++'): continue
        path = root / entry['path']
        text = path.read_text(encoding='latin-1')
        masked = NONCODE.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)
        lines = text.splitlines()
        for match in re.finditer(r'(?<![\w])-\s*1\b', masked):
            number = text.count('\n', 0, match.start()) + 1
            begin = masked.rfind('\n', 0, match.start()) + 1
            code = masked[begin:masked.find('\n', match.start()) if '\n' in masked[match.start():] else len(masked)]
            rows.append({'path':entry['path'], 'line':number,
                         'category':classify(code, masked[begin:match.start()].rstrip()),
                         'source':lines[number - 1].strip(), 'status':'needs_review'})
    report = {'policy':'Candidates only: retain documented error contracts, arithmetic, indices and protocol values unless a reviewed replacement is justified.',
              'counts':dict(sorted(collections.Counter(r['category'] for r in rows).items())),
              'records':rows}
    target = root / 'upgrade_data/sentinel_candidates.json'
    target.write_text('{\n  "policy": ' + json.dumps(report['policy']) + ',\n  "counts": ' + json.dumps(report['counts']) + ',\n  "records": [\n' + ',\n'.join('    ' + json.dumps(r) for r in rows) + '\n  ]\n}\n')
    print(json.dumps({'total':len(rows), 'counts':report['counts']}))


if __name__ == '__main__':
    main()
