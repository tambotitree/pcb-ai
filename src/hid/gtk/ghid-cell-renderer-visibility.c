/*!
 * \file src/hid/gtk/ghid-cell-renderer-visibility.c
 *
 * \brief Implementation of GtkCellRenderer for layer visibility toggler.
 *
 * For details on the functions implemented here, see the Gtk
 * documentation for the GtkCellRenderer object, which defines
 * the interface we are implementing.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#include <gtk/gtk.h>
#include "global.h"
#include "ghid-cell-renderer-visibility.h"

enum { TOGGLED, LAST_SIGNAL };
static guint toggle_cell_signals[LAST_SIGNAL];
enum { PROP_ACTIVE = 1, PROP_COLOR };

struct _GHidCellRendererVisibility
{
  GtkCellRenderer parent;
  gboolean active;
  gchar *color;
};

struct _GHidCellRendererVisibilityClass
{
  GtkCellRendererClass parent_class;
  void (*toggled) (GHidCellRendererVisibility *cell, const gchar *path);
};

G_DEFINE_TYPE (GHidCellRendererVisibility, ghid_cell_renderer_visibility,
               GTK_TYPE_CELL_RENDERER)

/*! \brief Calculate swatch size using GTK3 border metrics and cell padding. */
static void
ghid_cell_renderer_visibility_get_size (GtkCellRenderer *cell,
                                       GtkWidget *widget,
                                       const GdkRectangle *cell_area,
                                       gint *x_offset, gint *y_offset,
                                       gint *width, gint *height)
{
  GtkStyleContext *context = gtk_widget_get_style_context (widget);
  GtkBorder border;
  gint xpad, ypad, w, h;
  gfloat xalign, yalign;

  gtk_style_context_get_border (context, gtk_widget_get_state_flags (widget), &border);
  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
  gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);
  w = VISIBILITY_TOGGLE_SIZE + 2 * xpad + border.left + border.right;
  h = VISIBILITY_TOGGLE_SIZE + 2 * ypad + border.top + border.bottom;
  if (width) *width = w;
  if (height) *height = h;
  if (x_offset) *x_offset = 0;
  if (y_offset) *y_offset = 0;
  if (cell_area)
    {
      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        xalign = 1.0f - xalign;
      if (x_offset) *x_offset = MAX (0, xalign * (cell_area->width - w));
      if (y_offset) *y_offset = MAX (0, yalign * (cell_area->height - h));
    }
}

/*! \brief Draw active/inactive swatches using GTK's borrowed Cairo context.
 * The caller owns cr. Preserve its graphics state and current path.
 */
static void
ghid_cell_renderer_visibility_render (GtkCellRenderer *cell, cairo_t *cr,
                                     GtkWidget *widget,
                                     const GdkRectangle *background_area,
                                     const GdkRectangle *cell_area,
                                     GtkCellRendererState flags)
{
  GHidCellRendererVisibility *pcb_cell = GHID_CELL_RENDERER_VISIBILITY (cell);
  GdkRectangle rect;
  GdkRGBA color = {0.0, 0.0, 0.0, 1.0};
  cairo_pattern_t *pattern;
  cairo_path_t *saved_path;
  gint xpad, ypad;

  /* GTK supplies this required argument; drawing is clipped to cell_area. */
  (void) background_area;
  ghid_cell_renderer_visibility_get_size (cell, widget, cell_area,
                                         &rect.x, &rect.y, &rect.width, &rect.height);
  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);
  rect.x += cell_area->x + xpad;
  rect.y += cell_area->y + ypad;
  rect.width -= 2 * xpad;
  rect.height -= 2 * ypad;
  if (rect.width <= 0 || rect.height <= 0)
    return;

  if (pcb_cell->color)
    {
      GdkRGBA parsed;
      if (gdk_rgba_parse (&parsed, pcb_cell->color)) color = parsed;
    }
  if (flags & GTK_CELL_RENDERER_PRELIT)
    {
      color.red = (4.0 * color.red + 1.0) / 5.0;
      color.green = (4.0 * color.green + 1.0) / 5.0;
      color.blue = (4.0 * color.blue + 1.0) / 5.0;
    }

  saved_path = cairo_copy_path (cr);
  cairo_save (cr);
  cairo_new_path (cr);
  gdk_cairo_rectangle (cr, cell_area);
  cairo_clip (cr);
  cairo_set_line_width (cr, 1.0);
  cairo_rectangle (cr, rect.x + 0.5, rect.y + 0.5, rect.width - 1, rect.height - 1);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_fill_preserve (cr);
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_stroke (cr);

  pattern = cairo_pattern_create_radial ((rect.width - 1.0) * 0.75 + rect.x + 0.5,
                                         (rect.height - 1.0) * 0.75 + rect.y + 0.5,
                                         0.0,
                                         (rect.width - 1.0) * 0.50 + rect.x + 0.5,
                                         (rect.height - 1.0) * 0.50 + rect.y + 0.5,
                                         (rect.width - 1.0) * 0.71);
  cairo_pattern_add_color_stop_rgba (pattern, 0.0,
                                     (color.red * 4.0 + 1.0) / 5.0,
                                     (color.green * 4.0 + 1.0) / 5.0,
                                     (color.blue * 4.0 + 1.0) / 5.0, color.alpha);
  cairo_pattern_add_color_stop_rgba (pattern, 1.0,
                                     color.red, color.green, color.blue, color.alpha);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
  if (pcb_cell->active)
    cairo_rectangle (cr, rect.x + 0.5, rect.y + 0.5, rect.width - 1, rect.height - 1);
  else
    {
      cairo_move_to (cr, rect.x + 1, rect.y + 1);
      cairo_rel_line_to (cr, rect.width / 2, 0);
      cairo_rel_line_to (cr, -rect.width / 2, rect.width / 2);
      cairo_close_path (cr);
    }
  cairo_fill (cr);
  cairo_restore (cr);
  cairo_new_path (cr);
  cairo_append_path (cr, saved_path);
  cairo_path_destroy (saved_path);
}

/*! \brief Emit the toggled path; GTK requires the unused event/geometry inputs. */
static gboolean
ghid_cell_renderer_visibility_activate (GtkCellRenderer *cell, GdkEvent *event,
                                       GtkWidget *widget, const gchar *path,
                                       const GdkRectangle *background_area,
                                       const GdkRectangle *cell_area,
                                       GtkCellRendererState flags)
{
  (void) event;
  (void) widget;
  (void) background_area;
  (void) cell_area;
  (void) flags;
  g_signal_emit (cell, toggle_cell_signals[TOGGLED], 0, path);
  return TRUE;
}

/*! \brief Read renderer properties and diagnose invalid property IDs. */
static void
ghid_cell_renderer_visibility_get_property (GObject *object, guint param_id,
                                           GValue *value, GParamSpec *pspec)
{
  GHidCellRendererVisibility *cell = GHID_CELL_RENDERER_VISIBILITY (object);
  switch (param_id)
    {
    case PROP_ACTIVE: g_value_set_boolean (value, cell->active); break;
    case PROP_COLOR: g_value_set_string (value, cell->color); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec); break;
    }
}

/*! \brief Store renderer properties; the renderer owns its color string. */
static void
ghid_cell_renderer_visibility_set_property (GObject *object, guint param_id,
                                           const GValue *value, GParamSpec *pspec)
{
  GHidCellRendererVisibility *cell = GHID_CELL_RENDERER_VISIBILITY (object);
  switch (param_id)
    {
    case PROP_ACTIVE: cell->active = g_value_get_boolean (value); break;
    case PROP_COLOR:
      g_free (cell->color);
      cell->color = g_value_dup_string (value);
      break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec); break;
    }
}

/*! \brief Release the owned color and chain up to GtkCellRenderer. */
static void
ghid_cell_renderer_visibility_finalize (GObject *object)
{
  GHidCellRendererVisibility *cell = GHID_CELL_RENDERER_VISIBILITY (object);
  g_free (cell->color);
  G_OBJECT_CLASS (ghid_cell_renderer_visibility_parent_class)->finalize (object);
}

/*! \brief Initialize a toggle-capable renderer with a deterministic default color. */
static void
ghid_cell_renderer_visibility_init (GHidCellRendererVisibility *cell)
{
  cell->color = g_strdup ("black");
  g_object_set (cell, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
}

/*! \brief Register typed GTK3 virtual methods, properties and the toggled signal. */
static void
ghid_cell_renderer_visibility_class_init (GHidCellRendererVisibilityClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);
  object_class->get_property = ghid_cell_renderer_visibility_get_property;
  object_class->set_property = ghid_cell_renderer_visibility_set_property;
  object_class->finalize = ghid_cell_renderer_visibility_finalize;
  cell_class->get_size = ghid_cell_renderer_visibility_get_size;
  cell_class->render = ghid_cell_renderer_visibility_render;
  cell_class->activate = ghid_cell_renderer_visibility_activate;
  g_object_class_install_property (object_class, PROP_ACTIVE,
    g_param_spec_boolean ("active", _("Visibility state"), _("Visibility of the layer"),
                          FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_COLOR,
    g_param_spec_string ("color", _("Layer color"), _("Layer color"),
                         "black", G_PARAM_READWRITE));
  toggle_cell_signals[TOGGLED] = g_signal_new ("toggled",
    G_OBJECT_CLASS_TYPE (object_class), G_SIGNAL_RUN_LAST,
    G_STRUCT_OFFSET (GHidCellRendererVisibilityClass, toggled),
    NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

/*! \brief Return a new renderer; normal GTK floating-reference rules apply. */
GtkCellRenderer *
ghid_cell_renderer_visibility_new (void)
{
  return g_object_new (GHID_CELL_RENDERER_VISIBILITY_TYPE, NULL);
}
