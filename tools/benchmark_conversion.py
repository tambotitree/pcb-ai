#!/usr/bin/env python3
"""Replace the deprecated synchronous window update in the redraw benchmark."""

from conversion import main
from drawing_conversion import replace_function

BENCHMARK = '''static int
Benchmark (int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED,
           Coord x G_GNUC_UNUSED, Coord y G_GNUC_UNUSED)
{
  guint64 frames = 0;
  gint64 start, elapsed;
  GtkWidget *drawing = gport->drawing_area;
  gint width = gtk_widget_get_allocated_width (drawing);
  gint height = gtk_widget_get_allocated_height (drawing);
  cairo_surface_t *surface;
  cairo_t *cr;

  if (!gtk_widget_get_realized (drawing) || width <= 0 || height <= 0)
    return 1;
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);
  if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
    {
      cairo_destroy (cr);
      cairo_surface_destroy (surface);
      return 1;
    }

  start = g_get_monotonic_time ();
  do
    {
      /* Rebuild the backing image, then complete GTK rendering offscreen. */
      ghid_invalidate_all ();
      gtk_widget_draw (drawing, cr);
      cairo_surface_flush (surface);
      ++frames;
      elapsed = g_get_monotonic_time () - start;
    }
  while (elapsed < 10 * G_TIME_SPAN_SECOND);

  printf (_("%g offscreen redraws per second\\n"),
          (double) frames * G_TIME_SPAN_SECOND / (double) elapsed);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  return 0;
}'''


def transform(source, root, relative, tool):
    """Convert the sole benchmark while keeping its HID action signature."""
    if relative != 'src/hid/gtk/gtkhid-main.c':
        return source, 'Outside the GTK benchmark.'
    text = source.decode()
    if 'gdk_window_process_updates (' not in text:
        return source, 'Benchmark conversion already applied.'
    text = replace_function(text, 'Benchmark', BENCHMARK)
    text = text.replace('N_("Report the amount of redraws per second.");',
                        'N_("Report completed offscreen redraws per second.");')
    text = text.replace('@noindent This action reports the number of redraws per second on the command\nline interface.',
                        '@noindent This action reports completed offscreen redraws per second on the\ncommand line. It measures rendering, not display presentation throughput.')
    return text.encode(), 'Synchronous offscreen benchmark; targeted compile pending.'


if __name__ == '__main__':
    raise SystemExit(main('benchmark', transform, 'python3'))
