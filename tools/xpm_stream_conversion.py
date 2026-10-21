#!/usr/bin/env python3
"""Migrate reviewed embedded XPM loaders to pcb-xpm.h; preview by default."""
import argparse
import difflib
import re
from pathlib import Path


def main():
    """Add explicit array lengths and retain the original embedded artwork."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    path = Path(__file__).resolve().parents[1] / 'src/hid/gtk/gui-top-window.c'
    source = path.read_text()
    if '#include "pcb-xpm.h"' in source:
        print('XPM stream conversion already applied.')
        return
    result = source.replace('#include "gui-icons-misc.data"',
                            '#include "gui-icons-misc.data"\n#include "pcb-xpm.h"')
    result = result.replace('  gchar **xpm;', '  gchar **xpm;\n  gsize xpm_rows;')
    result, count = re.subn(r'(\{NULL, NULL, 0, 0, N_\("[^"]+"\), \w+, )(\w+),',
                           r'\1\2, G_N_ELEMENTS (\2),', result)
    if count != 14:
        raise ValueError(f'Expected 14 reviewed mode icons; found {count}')
    result = result.replace('gdk_pixbuf_new_from_xpm_data ((const char **) mb->xpm)',
                            'pcb_pixbuf_from_xpm (mb->xpm, mb->xpm_rows)')
    result = result.replace('      g_object_unref (pixbuf);',
                            '      if (pixbuf != NULL)\n        g_object_unref (pixbuf);')
    result = result.replace('  icon = gdk_pixbuf_new_from_xpm_data ((const gchar **) icon_bits);\n  gtk_window_set_default_icon (icon);',
                            '  icon = pcb_pixbuf_from_xpm (icon_bits, G_N_ELEMENTS (icon_bits));\n  if (icon != NULL)\n    {\n      gtk_window_set_default_icon (icon);\n      g_object_unref (icon);\n    }')
    print(''.join(difflib.unified_diff(source.splitlines(True), result.splitlines(True),
                                    fromfile=str(path), tofile=str(path))), end='')
    if args.apply:
        path.write_text(result)


if __name__ == '__main__':
    main()
