#!/usr/bin/env python3
"""Remove optional legacy window-manager class hints from GTK source."""

from conversion import main
from dialog_layout_conversion import calls


def transform(source, root, relative, tool):
    """Remove whole wmclass statements, preserving titles and window relationships."""
    if not relative.startswith('src/hid/gtk/') or not relative.endswith('.c'):
        return source, 'Outside GTK window source.'
    text = source.decode()
    edits = []
    names = ('gtk_window_set_wmclass', 'gtk_window_set_skip_pager_hint',
             'gtk_window_set_skip_taskbar_hint', 'gtk_tree_view_set_rules_hint')
    for name in names:
        for start, end, args in calls(text, name):
            if name == 'gtk_window_set_wmclass':
                valid = len(args) == 3 and all(a.startswith('"') and a.endswith('"') for a in args[1:])
            else:
                valid = len(args) == 2 and args[1] in ('TRUE', 'FALSE')
            if not valid:
                raise ValueError('Desktop hint arguments need review: ' + relative)
            stop = end
            while text[stop:stop + 1] in (' ', '\t'): stop += 1
            if text[stop:stop + 1] != ';':
                raise ValueError('Expected a standalone desktop hint statement: ' + relative)
            begin = text.rfind('\n', 0, start) + 1
            if text[begin:start].strip():
                raise ValueError('Desktop hint shares a statement line: ' + relative)
            stop += 1
            if text[stop:stop + 1] == '\n': stop += 1
            edits.append((begin, stop))
    for start, end in sorted(edits, reverse=True):
        text = text[:start] + text[end:]
    return text.encode(), f'{len(edits)} optional desktop hints removed.'


if __name__ == '__main__':
    raise SystemExit(main('desktop-hints', transform, 'python3'))
