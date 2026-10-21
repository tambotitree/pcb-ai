"""GTK3 drawing-buffer, color, cursor and thumbnail lifecycle replacements."""

CONFIGURE = r'''gboolean
ghid_port_drawing_area_configure_event_cb (GtkWidget *widget,
                                         GdkEventConfigure *event, GHidPort *port)
{
  static gboolean first;
  (void) widget;
  if (event->width <= 0 || event->height <= 0) return FALSE;
  port->width = event->width; port->height = event->height;
  if (port->pixmap) cairo_surface_destroy (port->pixmap);
  port->pixmap = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, port->width, port->height);
  port->drawable = port->pixmap;
  ghid_map_color_string (Settings.BackgroundColor, &port->bg_color);
  ghid_map_color_string (Settings.OffLimitColor, &port->offlimits_color);
  ghid_drawing_area_configure_hook (port);
  if (!first) { first = TRUE; PCBChanged (0, NULL, 0, 0); }
  ghid_port_ranges_scale (); ghid_invalidate_all ();
  return FALSE;
}'''

COLOR_MAP = r'''void
ghid_map_color_string (char *text, GdkRGBA *color)
{
  if (color && (!text || !gdk_rgba_parse (color, text)))
    *color = (GdkRGBA) {1, 1, 1, 1};
}'''

COLOR_NAME = r'''gchar *
ghid_get_color_name (GdkRGBA *color)
{
  if (!color) return g_strdup ("#000000");
  return g_strdup_printf ("#%02x%02x%02x",
    (unsigned int) (CLAMP (color->red, 0, 1) * 255 + 0.5),
    (unsigned int) (CLAMP (color->green, 0, 1) * 255 + 0.5),
    (unsigned int) (CLAMP (color->blue, 0, 1) * 255 + 0.5));
}'''

CURSOR = r'''static GdkCursorType
gport_set_cursor (GdkCursorType shape)
{
  GdkCursorType previous = gport->X_cursor_shape;
  GdkCursor *custom = NULL;
  GdkWindow *window;
  if (!gport->drawing_area) return GDK_X_CURSOR;
  window = gtk_widget_get_window (gport->drawing_area);
  if (!window) return GDK_X_CURSOR;
  if (shape == previous) return shape;
  if (shape == CUSTOM_CURSOR_CLOCKWISE) custom = XC_clock_cursor;
  else if (shape == CUSTOM_CURSOR_DRAG) custom = XC_hand_cursor;
  else if (shape == CUSTOM_CURSOR_LOCK) custom = XC_lock_cursor;
  g_clear_object (&gport->X_cursor);
  if (custom) gport->X_cursor = g_object_ref (custom);
  else gport->X_cursor = gdk_cursor_new_for_display (gtk_widget_get_display (gport->drawing_area),
                                                   shape > GDK_LAST_CURSOR ? GDK_LEFT_PTR : shape);
  gdk_window_set_cursor (window, gport->X_cursor);
  gport->X_cursor_shape = shape;
  return previous;
}'''

ICONS = r'''static void
ghid_init_icons (GHidPort *port)
{
  GdkDisplay *display = gtk_widget_get_display (port->top_window);
  g_clear_object (&XC_clock_cursor); g_clear_object (&XC_hand_cursor); g_clear_object (&XC_lock_cursor);
  XC_clock_cursor = pcb_cairo_cursor_from_bits (display, rotateIcon_bits, rotateMask_bits,
                                                rotateIcon_width, rotateIcon_height, 8, 8);
  XC_hand_cursor = pcb_cairo_cursor_from_bits (display, handIcon_bits, handMask_bits,
                                               handIcon_width, handIcon_height, 8, 8);
  XC_lock_cursor = pcb_cairo_cursor_from_bits (display, lockIcon_bits, lockMask_bits,
                                               lockIcon_width, lockIcon_height, 8, 8);
}'''

DRC_RENDER = r'''static void
ghid_violation_renderer_render (GtkCellRenderer *cell, cairo_t *cr, GtkWidget *widget,
                                const GdkRectangle *background_area,
                                const GdkRectangle *cell_area, GtkCellRendererState flags)
{
  GhidViolationRenderer *renderer = GHID_VIOLATION_RENDERER (cell);
  GhidDrcViolation *violation = renderer->violation;
  GdkRectangle text_area = *cell_area;
  int size = VIOLATION_PIXMAP_PIXEL_SIZE - 2 * VIOLATION_PIXMAP_PIXEL_BORDER;
  text_area.width = MAX (0, text_area.width - VIOLATION_PIXMAP_PIXEL_SIZE);
  GTK_CELL_RENDERER_CLASS (ghid_violation_renderer_parent_class)->render (
    cell, cr, widget, background_area, &text_area, flags);
  if (!violation) return;
  if (!violation->pixmap)
    {
      cairo_surface_t *surface = ghid_render_pixmap (violation->v->x, violation->v->y,
        VIOLATION_PIXMAP_PCB_SIZE / size, size, size, 32);
      g_object_set (violation, "pixmap", surface, NULL);
      if (surface) cairo_surface_destroy (surface);
    }
  if (!violation->pixmap) return;
  cairo_save (cr);
  cairo_rectangle (cr, cell_area->x, cell_area->y, cell_area->width, cell_area->height);
  cairo_clip (cr);
  cairo_set_source_surface (cr, violation->pixmap,
    text_area.x + text_area.width + VIOLATION_PIXMAP_PIXEL_BORDER,
    text_area.y + VIOLATION_PIXMAP_PIXEL_BORDER);
  cairo_paint (cr); cairo_restore (cr);
}'''
