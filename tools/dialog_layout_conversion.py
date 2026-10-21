#!/usr/bin/env python3
"""Convert reviewed GTK dialog table, label alignment and standard button patterns."""

import re

from c_conversion import NONCODE
from conversion import main


def calls(text, name):
    """Yield call spans and arguments, balancing nested expressions and literals."""
    masked = NONCODE.sub(lambda m: ' ' * len(m[0]), text)
    for match in re.finditer(r'\b' + name + r'\s*\(', masked):
        start = match.end()
        depth = 1
        args = []
        pos = start
        while depth:
            char = masked[pos]
            if char == '(':
                depth += 1
            elif char == ')':
                depth -= 1
            if (char == ',' and depth == 1) or depth == 0:
                args.append(text[start:pos].strip())
                start = pos + 1
            pos += 1
        yield match.start(), pos, args


def transform(source, root, relative, tool):
    """Apply confirmed patterns across GTK C sources without changing literals."""
    if not relative.startswith('src/hid/gtk/') or not relative.endswith('.c'):
        return source, 'Outside GTK implementation scope.'
    text = source.decode()
    edits = []
    count = 0
    for start, end, args in calls(text, 'g_object_new'):
        if args[0] != 'GTK_TYPE_ALIGNMENT': continue
        clean = [re.sub(r'/\*.*?\*/', '', arg, flags=re.S).strip() for arg in args]
        expected = ['GTK_TYPE_ALIGNMENT', '"left-padding"', '5', '"right-padding"', '5',
                    '"top-padding"', '5', '"bottom-padding"', '5', '"xscale"', '1.0',
                    '"yscale"', '1.0', '"xalign"', '0.5', '"yalign"', '0.5', 'NULL']
        if clean != expected:
            raise ValueError('Alignment property set needs review: ' + relative)
        edits.append((start, end, '''g_object_new (GTK_TYPE_BOX,
                                         "orientation", GTK_ORIENTATION_VERTICAL,
                                         "margin-start", 5, "margin-end", 5,
                                         "margin-top", 5, "margin-bottom", 5,
                                         NULL)'''))
    for start, end, args in calls(text, 'gtk_table_new'):
        if len(args) != 3 or args[2] not in ('FALSE', '0'):
            raise ValueError('Unreviewed homogeneous table: ' + relative)
        edits.append((start, end, 'gtk_grid_new ()'))
    for start, end, args in calls(text, 'gtk_misc_set_alignment'):
        match = re.fullmatch(r'GTK_MISC\s*\((.*)\)', args[0], re.S)
        if not match or len(args) != 3:
            raise ValueError('Unrecognized label alignment: ' + relative)
        widget = match[1]
        replacement = f'gtk_label_set_xalign (GTK_LABEL ({widget}), {args[1]});\n'
        replacement += f'  gtk_label_set_yalign (GTK_LABEL ({widget}), {args[2]})'
        edits.append((start, end, replacement))
    for start, end, args in calls(text, 'gtk_table_resize'):
        # GtkGrid creates rows as children are attached; no capacity hint needed.
        edits.append((start, end, '(void) 0'))
    for start, end, replacement in sorted(edits, reverse=True):
        text = text[:start] + replacement + text[end:]
    count += len(edits)
    mapping = {
        'gtk_table_attach_defaults': 'pcb_grid_attach_expanding',
        'gtk_table_attach': 'pcb_grid_attach_bounds',
        'GTK_TABLE': 'GTK_GRID',
        'gtk_table_set_col_spacings': 'gtk_grid_set_column_spacing',
        'gtk_table_set_row_spacings': 'gtk_grid_set_row_spacing',
        'GTK_STOCK_OK': '_("_OK")',
        'GTK_STOCK_CANCEL': '_("_Cancel")',
    }
    masked = NONCODE.sub(lambda m: ' ' * len(m[0]), text)
    changes = []
    for token, replacement in mapping.items():
        for match in re.finditer(r'\b' + token + r'\b', masked):
            changes.append((match.start(), match.end(), replacement))
    for start, end, replacement in sorted(changes, reverse=True):
        text = text[:start] + replacement + text[end:]
    count += len(changes)
    text = re.sub(r'gtk_button_new_from_stock\s*\(\s*(_\("_(?:OK|Cancel)"\))\s*\)',
                  r'gtk_button_new_with_mnemonic (\1)', text)
    text = text.replace('(GtkAttachOptions)', '(guint)')
    if 'pcb_grid_attach_' in text and '#include "pcb-grid.h"' not in text:
        text = text.replace('#include "gui.h"', '#include "gui.h"\n#include "pcb-grid.h"', 1)
    return text.encode(), f'{count} dialog layout/label replacements; compile pending.'


if __name__ == '__main__':
    raise SystemExit(main('dialog-layout', transform, 'python3'))
