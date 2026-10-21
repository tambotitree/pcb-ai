#!/usr/bin/env python3
"""Convert confirmed GTK pointer queries and warps to explicit device access."""

from conversion import main
from dialog_layout_conversion import calls


def transform(source, root, relative, tool):
    """Rewrite pointer call sites while preserving window/screen coordinate spaces."""
    if not relative.startswith('src/hid/gtk/') or not relative.endswith('.c'):
        return source, 'Outside GTK implementation scope.'
    text = source.decode()
    edits = []
    rules = (('gdk_window_get_pointer', 'pcb_pointer_query_window', 4),
             ('gdk_display_warp_pointer', 'pcb_pointer_warp', 4),
             ('gdk_display_get_pointer', 'pcb_pointer_query_screen', 5))
    for old, new, count in rules:
        for start, end, args in calls(text, old):
            if len(args) != count:
                raise ValueError('Unexpected pointer call: ' + relative)
            if old == 'gdk_display_get_pointer':
                if args[-1] != 'NULL':
                    raise ValueError('Display modifier query needs separate review: ' + relative)
                args = args[:-1]
            edits.append((start, end, new + ' (' + ', '.join(args) + ')'))
    for start, end, replacement in sorted(edits, reverse=True):
        text = text[:start] + replacement + text[end:]
    if edits and '#include "pcb-pointer.h"' not in text:
        if '#include "gui.h"' not in text:
            raise ValueError('No reviewed include insertion point: ' + relative)
        text = text.replace('#include "gui.h"', '#include "gui.h"\n#include "pcb-pointer.h"', 1)
    return text.encode(), f'{len(edits)} pointer calls converted; targeted compilation pending.'


if __name__ == '__main__':
    raise SystemExit(main('pointer', transform, 'python3'))
