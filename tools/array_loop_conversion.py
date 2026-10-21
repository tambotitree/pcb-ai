#!/usr/bin/env python3
"""Use unsigned array indices in four reviewed GTK table iteration functions."""

import re

from conversion import main
from hid_initializer_conversion import closing_brace, mask

RULES = {
    'src/hid/gtk/gui-command-window.c': ('ghid_command_window_show', 'command_ref_text'),
    'src/hid/gtk/gui-keyref-window.c': ('ghid_keyref_window_show', 'key_ref_text'),
    'src/hid/gtk/gui-config.c': ('config_layers_tab_create', 'layer_info_text'),
    'src/hid/gtk/gui-top-window.c': ('ghid_load_menus', 'ghid_hotkey_actions'),
}


def layer_entry_indices(text):
    """Match two reviewed layer-entry counters to LayerGroupType.Number."""
    rules = (
        ('make_layer_group_string', '  gint group, entry, layer;',
         '  gint group, layer;\n  Cardinal entry;'),
        ('config_layer_group_button_state_update', '  gint g, i;',
         '  gint g;\n  Cardinal i;'),
    )
    for name, old, new in rules:
        match = re.search(r'^' + name + r'\s*\([^;{}]*\)\s*\{', mask(text), re.M)
        if not match: raise ValueError('Layer-entry function changed: ' + name)
        end = closing_brace(mask(text), match.end() - 1) + 1
        body = text[match.start():end].replace(old, new, 1)
        text = text[:match.start()] + body + text[end:]
    return text


def transform(source, root, relative, tool):
    """Change only verified array-only indices, preserving other signed counters."""
    if relative not in RULES:
        return source, 'No reviewed array-only loop.'
    text = source.decode()
    name, array = RULES[relative]
    match = re.search(r'^' + name + r'\s*\([^;{}]*\)\s*\{', mask(text), re.M)
    if not match: raise ValueError('Function definition changed: ' + name)
    end = closing_brace(mask(text), match.end() - 1) + 1
    body = text[match.start():end]
    body = re.sub(r'^(  )(?:gint|int) i;', r'\1gsize i;', body, count=1, flags=re.M)
    if array == 'ghid_hotkey_actions':
        body = body.replace('i < N_HOTKEY_ACTIONS', 'i < G_N_ELEMENTS (ghid_hotkey_actions)')
    else:
        body = re.sub(r'sizeof\s*\(' + array + r'\)\s*/\s*sizeof\s*\(gchar\s*\*\)',
                      'G_N_ELEMENTS (' + array + ')', body)
    text = text[:match.start()] + body + text[end:]
    if relative == 'src/hid/gtk/gui-config.c':
        text = layer_entry_indices(text)
    return text.encode(), 'Reviewed array/count indices use the matching unsigned type.'


if __name__ == '__main__':
    raise SystemExit(main('array-loops', transform, 'python3'))
