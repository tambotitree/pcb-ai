#!/usr/bin/env python3
"""Remove the remaining stock dialog assets and desktop-dependent button order."""

import re

from c_conversion import NONCODE
from conversion import main
from dialog_layout_conversion import calls


def transform(source, root, relative, tool):
    """Keep dialog commands as text while dropping stock lookup and policy calls."""
    if not relative.startswith('src/hid/gtk/') or not relative.endswith('.c'):
        return source, 'Outside GTK dialog source.'
    text = source.decode()
    edits = []
    for start, end, args in calls(text, 'gtk_dialog_set_alternative_button_order'):
        if not all(a.startswith('GTK_RESPONSE_') or a == '-1' for a in args[1:]):
            raise ValueError('Alternative ordering has unreviewed argument expressions')
        if text[end:end + 1] != ';': raise ValueError('Expected standalone ordering call')
        edits.append((start, end + 1, ''))
    if relative == 'src/hid/gtk/gui-library-window.c':
        labels = {'GTK_STOCK_CLEAR': '_Clear', 'GTK_STOCK_REFRESH': '_Refresh'}
        for start, end, args in calls(text, 'gtk_image_new_from_stock'):
            if len(args) != 2 or args[0] not in labels:
                raise ValueError('Unreviewed library stock image')
            edits.append((start, end, 'gtk_label_new_with_mnemonic (_("' + labels[args[0]] + '"))'))
    masked = NONCODE.sub(lambda m: ' ' * len(m[0]), text)
    for old, label in (('GTK_STOCK_SAVE', '_Save'), ('GTK_STOCK_CLOSE', '_Close')):
        for match in re.finditer(r'\b' + old + r'\b', masked):
            edits.append((match.start(), match.end(), '_("' + label + '")'))
    for start, end, replacement in sorted(edits, reverse=True):
        text = text[:start] + replacement + text[end:]
    text = text.replace('  /* Set the alternative button order (ok, cancel, help) for other systems */\n', '')
    if relative == 'src/hid/gtk/gui-top-window.c':
        text = text.replace('''  button_image = gtk_image_new_from_stock (GTK_STOCK_REFRESH,
                                           GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (GTK_BUTTON (button), button_image);
''', '')
        text = text.replace('''  icon = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING,
                                   GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start (GTK_BOX (content_area),
                      icon, FALSE, FALSE, 0);
''', '')
        text = text.replace('''  GtkWidget *button;
  GtkWidget *button_image;
  GtkWidget *icon;
  GtkWidget *label;''', '  GtkWidget *label;', 1)
        text = text.replace('  button = gtk_info_bar_add_button', '  gtk_info_bar_add_button', 1)
    return text.encode(), 'Remaining stock dialogs use text; application response IDs unchanged.'


if __name__ == '__main__':
    raise SystemExit(main('dialog-stock', transform, 'python3'))
