#!/usr/bin/env python3
"""Remove obsolete tearoff menu construction from inventoried C sources."""

import re

from c_conversion import NONCODE
from conversion import main


def transform(source, root, relative, tool):
    """Remove confirmed GTK/Motif tearoff statements; preserve ordinary menus."""
    if not relative.startswith('src/hid/') or not relative.endswith('.c'):
        return source, 'No menu implementation selected.'
    text = source.decode('utf-8')
    masked = NONCODE.sub(lambda m: ''.join('\n' if c == '\n' else ' '
                                         for c in m[0]), text)
    patterns = (
        r'^[ \t]*GtkWidget \*tearoff = gtk_tearoff_menu_item_new \(\);\n',
        r'^[ \t]*gtk_menu_shell_append \(GTK_MENU_SHELL \(submenu\), tearoff\);\n',
        r'^[ \t]*stdarg \(XmNtearOffModel, XmTEAR_OFF_ENABLED\);\n',
    )
    edits = [m.span() for pattern in patterns
             for m in re.finditer(pattern, masked, re.MULTILINE)]
    for start, end in sorted(edits, reverse=True):
        text = text[:start] + text[end:]
    text = re.sub(r'^[ \t]*/\* add tearoff to menu \*/\n', '', text,
                  flags=re.MULTILINE)
    return text.encode(), f'{len(edits)} tearoff statements removed; compile pending.'


if __name__ == '__main__':
    raise SystemExit(main('menus', transform, 'python3'))
