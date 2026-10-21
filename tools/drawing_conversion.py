#!/usr/bin/env python3
"""One-time coordinated GTK2 drawable-to-Cairo migration for the non-GL backend."""
import re
from c_conversion import NONCODE
from conversion import main
from drawing_backend_rules import FUNCTIONS
from drawing_ui_rules import CONFIGURE, COLOR_MAP, COLOR_NAME, CURSOR, ICONS, DRC_RENDER

def replace_function(text, name, replacement):
  """Replace one complete C function, balancing braces outside literals/comments."""
  masked = NONCODE.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)
  match = re.search(r'(?m)^' + name + r'\s*\([^;{}]*\)\s*\{', masked)
  if not match:
    raise ValueError('Expected function definition: ' + name)
  start = masked.rfind('\n', 0, match.start() - 1) + 1
  brace = masked.index('{', match.start())
  depth = 1
  end = brace + 1
  while depth:
    if masked[end] == '{': depth += 1
    if masked[end] == '}': depth -= 1
    end += 1
  return text[:start] + replacement + text[end:]

def backend(text):
  """Preserve PCB geometry math while replacing graphics state and presentation."""
  for name, code in FUNCTIONS.items():
    text = replace_function(text, name, code)
  text = replace_function(text, 'ghid_use_mask', r'''void
ghid_use_mask (enum mask_mode mode)
{
  render_priv *priv = gport->render_priv;
  GdkRGBA white = {1, 1, 1, 1}, clear = {0, 0, 0, 1};
  if (!gport->pixmap || mode == cur_mask) return;
  if (mode == HID_MASK_BEFORE) { g_return_if_reached (); }
  if (mode == HID_MASK_CLEAR)
    {
      if (!gport->mask)
        gport->mask = cairo_image_surface_create (CAIRO_FORMAT_A8, gport->width, gport->height);
      if (!priv->mask_gc) priv->mask_gc = pcb_cairo_gc_new (gport->mask);
      set_clip (priv, priv->mask_gc);
      pcb_cairo_gc_set_foreground (priv->mask_gc, &white);
      pcb_cairo_draw_rectangle (gport->mask, priv->mask_gc, TRUE, 0, 0, gport->width, gport->height);
      pcb_cairo_gc_set_foreground (priv->mask_gc, &clear);
      gport->drawable = gport->mask;
    }
  else gport->drawable = gport->pixmap;
  cur_mask = mode;
}''')
  text = text.replace('#include "gui.h"', '#include "gui.h"\n#include "pcb-cairo.h"')
  text = text.replace('static void draw_lead_user (render_priv *priv);',
                      'static void draw_lead_user (render_priv *priv);\nstatic void set_special_grid_color (void);')
  for old, new in (
    ('GdkGC', 'PcbCairoGC'), ('GdkColor', 'GdkRGBA'), ('GdkPoint', 'PcbCairoPoint'),
    ('gdk_gc_new', 'pcb_cairo_gc_new'), ('gdk_gc_set_foreground','pcb_cairo_gc_set_foreground'),
    ('gdk_gc_set_clip_rectangle','pcb_cairo_gc_set_clip_rectangle'),
    ('gdk_gc_set_clip_mask','pcb_cairo_gc_set_clip_mask'),
    ('gdk_gc_set_function','pcb_cairo_gc_set_function'),
    ('gdk_gc_set_line_attributes','pcb_cairo_gc_set_line_attributes'),
    ('gdk_draw_line','pcb_cairo_draw_line'), ('gdk_draw_arc','pcb_cairo_draw_arc'),
    ('gdk_draw_rectangle','pcb_cairo_draw_rectangle'), ('gdk_draw_polygon','pcb_cairo_draw_polygon'),
    ('GDK_CAP_ROUND','CAIRO_LINE_CAP_ROUND'), ('GDK_JOIN_ROUND','CAIRO_LINE_JOIN_ROUND'),
    ('GDK_CAP_PROJECTING','CAIRO_LINE_CAP_SQUARE'), ('GDK_JOIN_MITER','CAIRO_LINE_JOIN_MITER'),
    ('GDK_CAP_BUTT','CAIRO_LINE_CAP_BUTT'), ('GDK_XOR','PCB_CAIRO_XOR'),
    ('GDK_COPY','PCB_CAIRO_COPY'), ('GdkCapStyle','cairo_line_cap_t'),
    ('GdkJoinStyle','cairo_line_join_t')):
    text = text.replace(old, new)
  text = re.sub(r'^\s*gdk_gc_set_clip_origin\s*\([^;]+;\n', '\n', text, flags=re.M)
  text = re.sub(r'^\s*gdk_gc_copy\s*\([^;]+;\n', '\n', text, flags=re.M)
  text = re.sub(r'^\s*GtkStyle \*style = gtk_widget_get_style[^;]+;\n', '\n', text, flags=re.M)
  text = re.sub(r'^\s*gdk_color_alloc\s*\([^;]+;\n', '\n', text, flags=re.M)
  text = text.replace('GDK_LINE_SOLID,', '')
  text = text.replace('g_object_unref (gc->gc);', 'pcb_cairo_gc_free (gc->gc);')
  text = text.replace('  g_free (gc);', '  g_free (gc->colorname);\n  g_free (gc);')
  text = text.replace('rv->colorname = Settings.BackgroundColor;', 'rv->colorname = g_strdup (Settings.BackgroundColor);')
  # The old grid color block both allocates a colormap entry and mutates 16-bit channels.
  text = re.sub(r'    if \(gdk_color_parse \(Settings.GridColor, &gport->grid_color\)\)\s*\{.*?\n    \}',
                '    set_special_grid_color ();', text, count=1, flags=re.S)
  text = text.replace('GdkWindow *window = gtk_widget_get_window (gport->drawing_area);',
                      'cairo_surface_t *window = gport->drawable;')
  text = text.replace('g_object_unref (port->mask);', 'cairo_surface_destroy (port->mask);')
  text = text.replace('gdk_flush ();', 'if (gport->drawing_area) gdk_display_flush (gtk_widget_get_display (gport->drawing_area));')
  text = text.replace('lead_color.pixel = 0;', 'lead_color.alpha = 1;')
  for component, suffix in (('red','R'), ('green','G'), ('blue','B')):
    text = re.sub(r'lead_color\.' + component + r'\s*=\s*\(int\)\(65535\. \* LEAD_USER_COLOR_' + suffix + r'\);',
                  'lead_color.' + component + ' = LEAD_USER_COLOR_' + suffix + ';', text)
  text = text.replace('gdk_pixbuf_render_to_drawable (pixbuf, gport->drawable, priv->bg_gc,',
                      'pcb_cairo_draw_pixbuf (gport->drawable, priv->bg_gc, pixbuf,')
  text = text.replace(', GDK_RGB_DITHER_NORMAL, 0, 0);', ');')
  text = text.replace('static int mask_seq = 0;', '')
  text = text.replace('void\nghid_invalidate_all ()', 'void\nghid_invalidate_all (void)')
  text = text.replace('gboolean\nlead_user_cb', 'static gboolean\nlead_user_cb')
  text = text.replace('  if (priv->lead_user)\n    ghid_invalidate_all ();', '  priv->lead_user = false;\n  if (gport->drawing_area) ghid_invalidate_all ();')
  # Empty GTK3 hooks retain explicit ABI signatures.
  text = text.replace('ghid_init_drawing_widget (GtkWidget *widget, GHidPort *port)\n{\n}',
                      'ghid_init_drawing_widget (GtkWidget *widget, GHidPort *port)\n{\n  (void) widget; (void) port;\n}')
  text = text.replace('ghid_port_drawing_realize_cb (GtkWidget *widget, gpointer data)\n{\n}',
                      'ghid_port_drawing_realize_cb (GtkWidget *widget, gpointer data)\n{\n  (void) widget; (void) data;\n}')
  return text

def transform(source, root, relative, tool):
  """Coordinate actual Cairo types, ownership, GTK3 callbacks and their consumers."""
  if not relative.startswith('src/hid/gtk/'):
    return source, 'Outside GTK drawing inputs.'
  text = source.decode('utf-8')
  if relative == 'src/hid/gtk/gtkhid-gdk.c':
    text = backend(text)
  elif relative == 'src/hid/gtk/gui.h':
    text = text.replace('#include <gtk/gtk.h>', '#include <gtk/gtk.h>\n#include <cairo-gobject.h>\n#include "pcb-cairo.h"')
    text = text.replace('GdkPixmap', 'cairo_surface_t').replace('GdkDrawable','cairo_surface_t')
    text = re.sub(r'  GdkColormap \*colormap;\n', '', text)
    text = re.sub(r'\bGdkColor\b', 'GdkRGBA', text)
    text = text.replace('GdkEventExpose *', 'cairo_t *')
    text = re.sub(r'extern cairo_surface_t \*XC_(hand|lock|clock)_source, \*XC_\1_mask;',
                  lambda m: 'extern GdkCursor *XC_' + m[1] + '_cursor;', text)
  elif relative == 'src/hid/gtk/gui-output-events.c':
    text = replace_function(text, 'ghid_port_drawing_area_configure_event_cb', CONFIGURE)
  elif relative == 'src/hid/gtk/gui-utils.c':
    text = replace_function(text, 'ghid_map_color_string', COLOR_MAP)
    text = replace_function(text, 'ghid_get_color_name', COLOR_NAME)
  elif relative == 'src/hid/gtk/gui-misc.c':
    text = replace_function(text, 'gport_set_cursor', CURSOR)
    text = re.sub(r'GdkPixmap \*XC_clock_source.*?\*XC_lock_mask;',
                  'GdkCursor *XC_clock_cursor, *XC_hand_cursor, *XC_lock_cursor;', text, count=1, flags=re.S)
  elif relative == 'src/hid/gtk/gui-top-window.c':
    text = replace_function(text, 'ghid_init_icons', ICONS)
    text = text.replace('"expose_event"', '"draw"')
  elif relative == 'src/hid/gtk/gui-pinout-preview.c':
    text = text.replace('gtk_widget_class->expose_event = ghid_pinout_preview_expose;',
                        'gtk_widget_class->draw = ghid_pinout_preview_expose;')
  elif relative == 'src/hid/gtk/gtkhid-main.c':
    text = text.replace('gdk_drawable_get_display (gport->drawable)',
                        'gtk_widget_get_display (gport->drawing_area)')
  elif relative == 'src/hid/gtk/gui-config.c':
    text = text.replace('GdkColor', 'GdkRGBA')
    text = text.replace('gtk_color_button_get_color (GTK_COLOR_BUTTON (button), &new_color)',
                        'gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &new_color)')
    text = text.replace('gtk_color_button_new_with_color', 'gtk_color_button_new_with_rgba')
  elif relative == 'src/hid/gtk/gui-drc-window.h':
    text = text.replace('GdkDrawable', 'cairo_surface_t')
  elif relative == 'src/hid/gtk/gui-drc-window.c':
    text = replace_function(text, 'ghid_violation_renderer_render', DRC_RENDER)
    text = text.replace('GdkDrawable', 'cairo_surface_t')
    text = text.replace('g_object_unref (gviolation->pixmap)', 'cairo_surface_destroy (gviolation->pixmap)')
    text = text.replace('(cairo_surface_t *)g_value_dup_object (value)', 'g_value_dup_boxed (value)')
    text = text.replace('g_param_spec_object ("pixmap"', 'g_param_spec_boxed ("pixmap"')
    text = text.replace('GDK_TYPE_DRAWABLE', 'CAIRO_GOBJECT_TYPE_SURFACE')
  else:
    return source, 'Outside the coordinated drawing migration.'
  return text.encode(), 'Cairo drawing types, image/mask ownership and GTK3 draw callback migration.'

if __name__ == '__main__':
  raise SystemExit(main('drawing', transform, 'python3'))
