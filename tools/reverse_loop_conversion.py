#!/usr/bin/env python3
"""Use unsigned countdown conditions in reviewed Cardinal reverse loops."""

import re

from conversion import main
from c_conversion import NONCODE


def transform(source, root, relative, tool):
    """Preserve reverse visit order without comparing unsigned counters to -1."""
    if relative not in ('src/macro.h', 'src/drill.c', 'src/rats.c'):
        return source, 'No reviewed unsigned reverse loop.'
    text = source.decode()
    masked = NONCODE.sub(lambda m: ' ' * len(m[0]), text)
    pattern = re.compile(r'for\s*\(([nlm])\s*=\s*((?:\(\w+\)|\w+)->\w+|MAX_ELEMENTNAMES)\s*-\s*1;\s*\1\s*!=\s*-1;\s*\1--\)')
    edits = [(m.start(), m.end(), f'for ({m[1]} = {m[2]}; {m[1]}-- > 0;)')
             for m in pattern.finditer(masked)]
    for start, end, candidate in reversed(edits):
        text = text[:start] + candidate + text[end:]
    if relative == 'src/drill.c':
        text = text.replace('if (n == -1)', 'if (n >= Drill->ElementN)')
    return text.encode(), f'{len(edits)} Cardinal reverse loops converted; order retained.'


if __name__ == '__main__':
    raise SystemExit(main('reverse-loops', transform, 'python3'))
