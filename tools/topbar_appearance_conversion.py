#!/usr/bin/env python3
"""One-time replacement of legacy desktop style copying; preview by default."""
import argparse
import difflib
from pathlib import Path

REPLACEMENT = '/* Application-owned top-bar appearance; independent of desktop theme copying. */\ntypedef struct\n{\n  GdkRGBA background;\n  GdkRGBA foreground;\n  GdkRGBA border;\n  guint padding_px;\n} PcbTopbarAppearance;\n\nstatic const PcbTopbarAppearance topbar_appearance = {\n  .background = {0.94, 0.94, 0.94, 1.0},\n  .foreground = {0.10, 0.10, 0.10, 1.0},\n  .border = {0.65, 0.65, 0.65, 1.0},\n  .padding_px = 4\n};\n\n/* Translate explicit appearance values into GTK3 styling for this window. */\nstatic void\napply_topbar_appearance (void)\n{\n  GtkWidget *widgets[] = {\n    ghidgui->top_bar_background,\n    ghidgui->menu_bar,\n    ghidgui->mode_toolbar,\n    ghidgui->cursor_position_relative_label,\n    ghidgui->cursor_position_absolute_label,\n    gtk_widget_get_parent (ghidgui->cursor_position_relative_label),\n    gtk_widget_get_parent (ghidgui->cursor_position_absolute_label),\n    ghidgui->grid_units_button,\n    ghidgui->grid_units_label\n  };\n  GtkCssProvider *provider;\n  GError *error = NULL;\n  gchar *background;\n  gchar *foreground;\n  gchar *border;\n  gchar *css;\n  gsize i;\n\n  background = gdk_rgba_to_string (&topbar_appearance.background);\n  foreground = gdk_rgba_to_string (&topbar_appearance.foreground);\n  border = gdk_rgba_to_string (&topbar_appearance.border);\n  css = g_strdup_printf (\n    ".pcb-topbar { background-image: none; background-color: %s; "\n    "color: %s; border-color: %s; padding: %upx; }",\n    background, foreground, border, topbar_appearance.padding_px);\n  provider = gtk_css_provider_new ();\n  if (gtk_css_provider_load_from_data (provider, css, -1, &error))\n    {\n      for (i = 0; i < G_N_ELEMENTS (widgets); i++)\n        {\n          GtkStyleContext *context = gtk_widget_get_style_context (widgets[i]);\n          gtk_style_context_add_class (context, "pcb-topbar");\n          gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider),\n                                         GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);\n        }\n    }\n  else\n    {\n      g_warning ("Cannot apply PCB top-bar appearance: %s", error->message);\n      g_clear_error (&error);\n    }\n  g_object_unref (provider);\n  g_free (css);\n  g_free (border);\n  g_free (foreground);\n  g_free (background);\n}\n\n'


def main():
    """Convert the reviewed top-bar block without touching image loading."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    path = root / 'src/hid/gtk/gui-top-window.c'
    original = path.read_text()
    if 'PcbTopbarAppearance' in original:
        print('Top-bar appearance already converted.')
        return
    start = original.index('static void\nget_widget_styles (')
    end = original.index('/*!\n * \\brief Create the top_window contents.', start)
    updated = original[:start] + REPLACEMENT + original[end:]
    updated = updated.replace('fix_topbar_theming (); /* Must be called after toolbar is created */',
                              'apply_topbar_appearance (); /* Toolbar and position widgets exist. */')
    print(''.join(difflib.unified_diff(original.splitlines(True), updated.splitlines(True),
                                    fromfile=str(path), tofile=str(path))), end='')
    if args.apply:
        path.write_text(updated)


if __name__ == '__main__':
    main()
