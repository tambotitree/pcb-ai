#!/usr/bin/env python3
"""Match layer iteration counters to the signed layer bounds in the PCB model."""

import re

from conversion import main

MACROS = {
    'ALLLINE_LOOP', 'ALLARC_LOOP', 'ALLPOLYGON_LOOP',
    'COPPERLINE_LOOP', 'COPPERARC_LOOP', 'COPPERPOLYGON_LOOP',
    'SILKLINE_LOOP', 'SILKARC_LOOP', 'SILKPOLYGON_LOOP', 'ALLTEXT_LOOP',
    'VISIBLELINE_LOOP', 'VISIBLEARC_LOOP', 'VISIBLETEXT_LOOP', 'VISIBLEPOLYGON_LOOP',
    'LAYER_LOOP', 'LAYER_TYPE_LOOP',
}


def transform(source, root, relative, tool):
    """Change layer indices only; object counts and reverse-loop sentinels remain unsigned."""
    if relative != 'src/macro.h':
        return source, 'Outside shared layer iteration declarations.'
    text = source.decode()
    definitions = list(re.finditer(r'^#define\s+(\w+)', text, re.M))
    edits = []
    seen = set()
    for i, match in enumerate(definitions):
        if match[1] not in MACROS: continue
        seen.add(match[1])
        end = definitions[i + 1].start() if i + 1 < len(definitions) else len(text)
        body = text[match.start():end]
        body, count = re.subn(r'\bCardinal([ \t]+)([ln]);', r'int\1\2;', body, count=1)
        if count: edits.append((match.start(), end, body))
    if seen != MACROS: raise ValueError('Layer macro inventory changed')
    for start, end, body in reversed(edits):
        text = text[:start] + body + text[end:]
    return text.encode(), f'{len(edits)} reviewed layer counters matched to int bounds.'


if __name__ == '__main__':
    raise SystemExit(main('layer-loops', transform, 'python3'))
