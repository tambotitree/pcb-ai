/* Exercise the real GTK3 renderer without constructing the full PCB window. */
#include <gtk/gtk.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "ghid-cell-renderer-visibility.h"

typedef struct { unsigned int count; char path[32]; } ToggleState;

/*! Record the actual path emitted by the renderer's activation callback. */
static void
on_toggled (GHidCellRendererVisibility *cell, const gchar *path, ToggleState *state)
{
  (void) cell;
  state->count++;
  g_strlcpy (state->path, path, sizeof state->path);
}

/*! Mark that the owned renderer reference was released and finalized. */
static void
on_finalized (gpointer data, GObject *object)
{
  gboolean *finalized = data;
  (void) object;
  *finalized = TRUE;
}

/*! Render into a caller-owned context and check graphics/path state survives. */
static void
draw (GtkCellRenderer *cell, GtkWidget *widget, GtkCellRendererState flags,
      unsigned char *pixels, const char *png)
{
  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 64, 64);
  cairo_t *cr = cairo_create (surface);
  GdkRectangle area = {10, 10, 32, 32};
  double red, green, blue, alpha;
  double x1, y1, x2, y2, after_x1, after_y1, after_x2, after_y2;
  int x, y, changed = 0;
  unsigned int *image;
  GtkCellRendererClass *klass = GTK_CELL_RENDERER_GET_CLASS (cell);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);
  cairo_set_line_width (cr, 7);
  cairo_set_source_rgba (cr, 0.1, 0.2, 0.3, 0.4);
  cairo_move_to (cr, 2, 3);
  cairo_line_to (cr, 6, 7);
  cairo_path_extents (cr, &x1, &y1, &x2, &y2);
  klass->render (cell, cr, widget, &area, &area, flags);
  assert (cairo_status (cr) == CAIRO_STATUS_SUCCESS);
  assert (cairo_get_line_width (cr) == 7);
  assert (cairo_pattern_get_rgba (cairo_get_source (cr), &red, &green, &blue, &alpha)
          == CAIRO_STATUS_SUCCESS);
  assert (fabs (red - 0.1) < 1e-9 && fabs (alpha - 0.4) < 1e-9);
  cairo_path_extents (cr, &after_x1, &after_y1, &after_x2, &after_y2);
  assert (x1 == after_x1 && y1 == after_y1 && x2 == after_x2 && y2 == after_y2);
  cairo_surface_flush (surface);
  assert (cairo_image_surface_get_stride (surface) == 64 * 4);
  image = (unsigned int *) cairo_image_surface_get_data (surface);
  for (y = 0; y < 64; y++)
    for (x = 0; x < 64; x++)
      {
        if (image[y * 64 + x] != 0xffffffffU) changed++;
        if (x < area.x || x >= area.x + area.width ||
            y < area.y || y >= area.y + area.height)
          assert (image[y * 64 + x] == 0xffffffffU);
      }
  assert (changed > 0);
  memcpy (pixels, image, 64 * 64 * 4);
  if (png) assert (cairo_surface_write_to_png (surface, png) == CAIRO_STATUS_SUCCESS);
  /* The renderer must not have destroyed our context. */
  cairo_set_source_rgb (cr, 0, 1, 0);
  assert (cairo_status (cr) == CAIRO_STATUS_SUCCESS);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

/*! Check properties, rendering variants, activation and finalization. */
int
main (int argc, char **argv)
{
  GtkCellRenderer *cell;
  GtkWidget *widget;
  GtkCssProvider *css;
  ToggleState toggled = {0, ""};
  gboolean finalized = FALSE;
  unsigned char active[64 * 64 * 4], inactive[64 * 64 * 4];
  unsigned char prelit[64 * 64 * 4], invalid[64 * 64 * 4], black[64 * 64 * 4];
  unsigned long normal_brightness = 0, prelit_brightness = 0;
  size_t i;
  gchar *color;
  char *png;

  if (!gtk_init_check (&argc, &argv)) return 77;
  assert (argc == 2);
  g_log_set_always_fatal (G_LOG_FATAL_MASK | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);
  widget = gtk_label_new ("fixture");
  g_object_ref_sink (widget);
  css = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css, "* { border: 0; padding: 0; }", -1, NULL);
  gtk_style_context_add_provider (gtk_widget_get_style_context (widget),
    GTK_STYLE_PROVIDER (css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css);
  cell = ghid_cell_renderer_visibility_new ();
  g_object_ref_sink (cell);
  g_object_weak_ref (G_OBJECT (cell), on_finalized, &finalized);
  gtk_cell_renderer_set_padding (cell, 1, 1);
  g_object_set (cell, "active", TRUE, "color", "#204080", NULL);
  g_object_get (cell, "color", &color, NULL);
  assert (strcmp (color, "#204080") == 0);
  g_free (color);
  png = g_build_filename (argv[1], "visibility-active.png", NULL);
  draw (cell, widget, 0, active, png);
  g_free (png);
  png = g_build_filename (argv[1], "visibility-prelit.png", NULL);
  draw (cell, widget, GTK_CELL_RENDERER_PRELIT, prelit, png);
  g_free (png);
  assert (memcmp (active, prelit, sizeof active) != 0);
  for (i = 0; i < sizeof active; i++)
    if (i % 4 != 3) { normal_brightness += active[i]; prelit_brightness += prelit[i]; }
  assert (prelit_brightness > normal_brightness);
  g_object_set (cell, "active", FALSE, NULL);
  png = g_build_filename (argv[1], "visibility-inactive.png", NULL);
  draw (cell, widget, 0, inactive, png);
  g_free (png);
  assert (memcmp (active, inactive, sizeof active) != 0);
  g_object_set (cell, "color", "not-a-color", NULL);
  draw (cell, widget, 0, invalid, NULL);
  g_object_set (cell, "color", "black", NULL);
  draw (cell, widget, 0, black, NULL);
  assert (memcmp (invalid, black, sizeof black) == 0);
  g_object_set (cell, "color", NULL, NULL);
  draw (cell, widget, 0, invalid, NULL);
  assert (memcmp (invalid, black, sizeof black) == 0);
  g_signal_connect (cell, "toggled", G_CALLBACK (on_toggled), &toggled);
  assert (GTK_CELL_RENDERER_GET_CLASS (cell)->activate (cell, NULL, widget, "4",
                                                       NULL, NULL, 0));
  assert (toggled.count == 1 && strcmp (toggled.path, "4") == 0);
  g_object_unref (cell);
  assert (finalized);
  g_object_unref (widget);
  return 0;
}
