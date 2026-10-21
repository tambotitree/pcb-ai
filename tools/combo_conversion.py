#!/usr/bin/env python3
"""Migrate GTK text combos without changing supported model-backed pickers."""

import re

from conversion import main
from dialog_layout_conversion import calls


def transform(source, root, relative, tool):
    """Convert text helpers and their required casts across GTK source files."""
    if not relative.startswith('src/hid/gtk/') or not relative.endswith('.c'):
        return source, 'Outside GTK source scope.'
    text = source.decode()
    edits = []
    rules = {
        'gtk_combo_box_entry_new_text': 'gtk_combo_box_text_new_with_entry',
        'gtk_combo_box_new_text': 'gtk_combo_box_text_new',
        'gtk_combo_box_append_text': 'gtk_combo_box_text_append_text',
        'gtk_combo_box_prepend_text': 'gtk_combo_box_text_prepend_text',
        'gtk_combo_box_remove_text': 'gtk_combo_box_text_remove',
        'gtk_combo_box_get_active_text': 'gtk_combo_box_text_get_active_text',
    }
    for old, new in rules.items():
        for start, end, args in calls(text, old):
            if old not in ('gtk_combo_box_entry_new_text', 'gtk_combo_box_new_text'):
                if not re.match(r'GTK_COMBO_BOX\s*\(', args[0]):
                    raise ValueError('Text operation without a reviewed combo cast: ' + relative)
                args[0] = re.sub(r'^GTK_COMBO_BOX\b', 'GTK_COMBO_BOX_TEXT', args[0])
            edits.append((start, end, new + ' (' + ', '.join(args) + ')'))
    for start, end, replacement in sorted(edits, reverse=True):
        text = text[:start] + replacement + text[end:]
    if relative == 'src/hid/gtk/gui-command-window.c':
        text = text.replace('  gint i;\n\n  if (!cmd',
                            '  gint i;\n  guint history_limit = ghidgui->history_size > 0 ? (guint) ghidgui->history_size : 0;\n\n  if (!cmd', 1)
        text = text.replace('if (g_list_length (history_list) > ghidgui->history_size)',
                            'while (g_list_length (history_list) > history_limit)')
        text = text.replace('g_list_nth_data (history_list, ghidgui->history_size)',
                            'g_list_nth_data (history_list, history_limit)')
        text = re.sub(r'(gtk_combo_box_text_remove\s*\(GTK_COMBO_BOX_TEXT\s*\(ghidgui->command_combo_box\),\s*)ghidgui->history_size',
                      r'\1(gint) history_limit', text)
        text = text.replace('g_object_ref (G_OBJECT (ghidgui->command_combo_box));',
                            'g_object_ref_sink (ghidgui->command_combo_box);')
    if relative == 'src/hid/gtk/gui-dialog-print.c':
        text = text.replace('enum_changed_cb (GtkWidget * combo_box, int *val)',
                            'enum_changed_cb (GtkComboBox *combo_box, int *val)')
        text = text.replace('gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box))',
                            'gtk_combo_box_get_active (combo_box)')
    return text.encode(), f'{len(edits)} text combo calls converted; model APIs preserved.'


if __name__ == '__main__':
    raise SystemExit(main('combos', transform, 'python3'))
