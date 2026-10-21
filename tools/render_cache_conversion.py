#!/usr/bin/env python3
"""Represent exporter line-cap cache values and validity with explicit types."""

import re

from conversion import main

PATHS = {'src/hid/ps/ps.c', 'src/hid/ps/eps.c', 'src/hid/gerber/gerber.c'}


def transform(source, root, relative, tool):
    """Replace only reviewed line-cap -1 sentinels; retain drawing enums."""
    text = source.decode('latin-1')
    if relative == 'src/hid/png/png.c':
        text = re.sub(r'^static int lastcap = -1;\n', '', text, flags=re.M)
        text = re.sub(r'^  lastcap = -1;\n', '', text, flags=re.M)
        return text.encode('latin-1'), 'Removed unused PNG cap cache; renderer uses its GC cap.'
    if relative not in PATHS or 'lastcap_valid' in text:
        return source, 'No unconverted line-cap cache.'
    text = re.sub(r'^([ \t]*)static int lastcap = -1;',
                  r'\1static EndCapStyle lastcap = Trace_Cap;\n\1static bool lastcap_valid = false;',
                  text, flags=re.M)
    text = text.replace('lastcap = lastcolor = -1;', 'lastcap_valid = false;\n      lastcolor = -1;')
    text = text.replace('lastcap = -1;', 'lastcap_valid = false;')
    text = text.replace('lastcap != ', '!lastcap_valid || lastcap != ')
    text = re.sub(r'^([ \t]*)lastcap = (gc->cap|Round_Cap);',
                  r'\1lastcap = \2;\n\1lastcap_valid = true;', text, flags=re.M)
    return text.encode('latin-1'), 'Cap cache uses EndCapStyle plus bool validity; compile pending.'


if __name__ == '__main__':
    raise SystemExit(main('render-cache', transform, 'python3'))
