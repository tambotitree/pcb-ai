"""Coordinated lifecycle replacements for the one-time Cairo migration."""

FUNCTIONS = {
"ghid_set_color": r'''void
ghid_set_color (hidGC gc, const char *name)
{
  GdkRGBA color;
  gchar *copy = g_strdup (name ? name : "magenta");
  g_free (gc->colorname); gc->colorname = copy;
  if (!gc->gc) return;
  if (!strcmp (copy, "erase")) color = gport->bg_color;
  else if (!strcmp (copy, "drill")) color = gport->offlimits_color;
  else if (!gdk_rgba_parse (&color, copy)) color = (GdkRGBA) {1, 1, 1, 1};
  if (gc->xor_mask) pcb_cairo_color_xor (&color, &color, &gport->bg_color);
  pcb_cairo_gc_set_foreground (gc->gc, &color);
}''',
"set_special_grid_color": r'''static void
set_special_grid_color (void)
{
  render_priv *priv = gport->render_priv;
  GdkRGBA color;
  if (!gdk_rgba_parse (&color, Settings.GridColor)) color = (GdkRGBA) {1, 1, 1, 1};
  pcb_cairo_color_xor (&gport->grid_color, &color, &gport->bg_color);
  if (priv->grid_gc) pcb_cairo_gc_set_foreground (priv->grid_gc, &gport->grid_color);
}''',
"use_gc": r'''static int
use_gc (hidGC gc)
{
  render_priv *priv = gport->render_priv;
  if (gc->me_pointer != &ghid_hid) g_error ("GC from another HID passed to GTK");
  if (!gport->drawable) return 0;
  if (!gc->gc) gc->gc = pcb_cairo_gc_new (gport->drawable);
  ghid_set_color (gc, gc->colorname);
  pcb_cairo_gc_set_line_attributes (gc->gc, Vz (gc->width), gc->cap, gc->join);
  pcb_cairo_gc_set_function (gc->gc, gc->xor_mask ? PCB_CAIRO_XOR : PCB_CAIRO_COPY);
  set_clip (priv, gc->gc);
  pcb_cairo_gc_set_clip_mask (gc->gc, cur_mask == HID_MASK_AFTER ? gport->mask : NULL);
  priv->u_gc = cur_mask == HID_MASK_CLEAR ? priv->mask_gc : gc->gc;
  return 1;
}''',
"ghid_init_renderer": r'''void
ghid_init_renderer (int *argc, char ***argv, GHidPort *port)
{
  render_priv *priv = g_new0 (render_priv, 1);
  (void) argc; (void) argv;
  port->render_priv = priv;
  priv->bg_gc = pcb_cairo_gc_new (NULL);
  priv->offlimits_gc = pcb_cairo_gc_new (NULL);
  ghid_map_color_string (Settings.BackgroundColor, &port->bg_color);
  ghid_map_color_string (Settings.OffLimitColor, &port->offlimits_color);
  priv->crosshair_gc = ghid_make_gc ();
  ghid_set_draw_xor (priv->crosshair_gc, 1);
}''',
"ghid_shutdown_renderer": r'''void
ghid_shutdown_renderer (GHidPort *port)
{
  render_priv *priv = port->render_priv;
  if (!priv) return;
  ghid_cancel_lead_user ();
  ghid_destroy_gc (priv->crosshair_gc);
  pcb_cairo_gc_free (priv->bg_gc); pcb_cairo_gc_free (priv->offlimits_gc);
  pcb_cairo_gc_free (priv->grid_gc); pcb_cairo_gc_free (priv->mask_gc);
  if (port->pixmap) cairo_surface_destroy (port->pixmap);
  if (port->mask) cairo_surface_destroy (port->mask);
  port->pixmap = port->mask = port->drawable = NULL;
  g_free (priv); port->render_priv = NULL;
}''',
"ghid_drawing_area_configure_hook": r'''void
ghid_drawing_area_configure_hook (GHidPort *port)
{
  render_priv *priv = port->render_priv;
  pcb_cairo_gc_set_foreground (priv->bg_gc, &port->bg_color);
  pcb_cairo_gc_set_foreground (priv->offlimits_gc, &port->offlimits_color);
  if (port->mask) { cairo_surface_destroy (port->mask); port->mask = NULL; }
  cur_mask = HID_MASK_OFF;
}''',
"ghid_screen_update": r'''void
ghid_screen_update (void)
{
  if (gport->drawing_area) gtk_widget_queue_draw (gport->drawing_area);
}''',
"ghid_drawing_area_expose_cb": r'''gboolean
ghid_drawing_area_expose_cb (GtkWidget *widget, cairo_t *cr, GHidPort *port)
{
  cairo_surface_t *present, *saved;
  cairo_t *copy;
  (void) widget;
  if (!port->pixmap) return FALSE;
  present = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, port->width, port->height);
  copy = cairo_create (present);
  cairo_set_source_surface (copy, port->pixmap, 0, 0); cairo_paint (copy); cairo_destroy (copy);
  saved = port->drawable; port->drawable = present;
  draw_crosshair (port->render_priv);
  port->drawable = saved;
  cairo_save (cr); cairo_set_source_surface (cr, present, 0, 0); cairo_paint (cr);
  cairo_restore (cr); cairo_surface_destroy (present);
  return FALSE;
}''',
"ghid_render_pixmap": r'''cairo_surface_t *
ghid_render_pixmap (int cx, int cy, double zoom, int width, int height, int depth)
{
  cairo_surface_t *surface, *saved_drawable = gport->drawable, *saved_mask = gport->mask;
  cairo_surface_t *saved_pixmap = gport->pixmap;
  view_data saved_view = gport->view;
  render_priv *priv = gport->render_priv;
  bool saved_clip = priv->clip;
  enum mask_mode saved_mode = cur_mask;
  int saved_width = gport->width, saved_height = gport->height;
  BoxType region;
  (void) depth;
  g_return_val_if_fail (width > 0 && height > 0 && zoom > 0, NULL);
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  gport->pixmap = surface; gport->drawable = surface; gport->mask = NULL; cur_mask = HID_MASK_OFF; priv->clip = false;
  gport->width = width; gport->height = height;
  gport->view.coord_per_px = zoom;
  gport->view.width = width * zoom; gport->view.height = height * zoom;
  gport->view.x0 = (gport->view.flip_x ? PCB->MaxWidth - cx : cx) - gport->view.width / 2;
  gport->view.y0 = (gport->view.flip_y ? PCB->MaxHeight - cy : cy) - gport->view.height / 2;
  pcb_cairo_gc_set_clip_rectangle (priv->bg_gc, NULL);
  pcb_cairo_draw_rectangle (surface, priv->bg_gc, TRUE, 0, 0, width, height);
  region.X1 = MAX (0, MIN (Px (0), Px (width + 1)));
  region.Y1 = MAX (0, MIN (Py (0), Py (height + 1)));
  region.X2 = MIN (PCB->MaxWidth, MAX (Px (0), Px (width + 1)));
  region.Y2 = MIN (PCB->MaxHeight, MAX (Py (0), Py (height + 1)));
  hid_expose_callback (&ghid_hid, &region, NULL);
  if (gport->mask) cairo_surface_destroy (gport->mask);
  gport->pixmap = saved_pixmap; gport->drawable = saved_drawable; gport->mask = saved_mask;
  gport->view = saved_view; gport->width = saved_width; gport->height = saved_height;
  cur_mask = saved_mode; priv->clip = saved_clip; set_clip (priv, priv->bg_gc);
  return surface;
}''',
"ghid_pinout_preview_expose": r'''gboolean
ghid_pinout_preview_expose (GtkWidget *widget, cairo_t *cr)
{
  GhidPinoutPreview *pinout = GHID_PINOUT_PREVIEW (widget);
  GtkAllocation allocation;
  cairo_surface_t *surface, *saved_drawable = gport->drawable, *saved_mask = gport->mask;
  cairo_surface_t *saved_pixmap = gport->pixmap;
  view_data saved_view = gport->view;
  enum mask_mode saved_mode = cur_mask;
  render_priv *priv = gport->render_priv;
  bool saved_clip = priv->clip;
  int saved_width = gport->width, saved_height = gport->height;
  Coord saved_max_width = PCB->MaxWidth, saved_max_height = PCB->MaxHeight;
  gtk_widget_get_allocation (widget, &allocation);
  if (allocation.width <= 0 || allocation.height <= 0 || !pinout->element) return FALSE;
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, allocation.width, allocation.height);
  gport->pixmap = surface; gport->drawable = surface; gport->mask = NULL; cur_mask = HID_MASK_OFF; priv->clip = false;
  gport->width = allocation.width; gport->height = allocation.height;
  gport->view.coord_per_px = MAX ((double) pinout->x_max / allocation.width,
                                 (double) pinout->y_max / allocation.height);
  if (gport->view.coord_per_px <= 0) gport->view.coord_per_px = 1;
  gport->view.width = allocation.width * gport->view.coord_per_px;
  gport->view.height = allocation.height * gport->view.coord_per_px;
  gport->view.x0 = (pinout->x_max - gport->view.width) / 2;
  gport->view.y0 = (pinout->y_max - gport->view.height) / 2;
  PCB->MaxWidth = pinout->x_max; PCB->MaxHeight = pinout->y_max;
  pcb_cairo_gc_set_clip_rectangle (priv->bg_gc, NULL);
  pcb_cairo_draw_rectangle (surface, priv->bg_gc, TRUE, 0, 0, allocation.width, allocation.height);
  hid_expose_callback (&ghid_hid, NULL, pinout->element);
  cairo_save (cr); cairo_set_source_surface (cr, surface, 0, 0); cairo_paint (cr); cairo_restore (cr);
  cairo_surface_destroy (surface);
  if (gport->mask) cairo_surface_destroy (gport->mask);
  gport->pixmap = saved_pixmap; gport->drawable = saved_drawable; gport->mask = saved_mask; gport->view = saved_view;
  gport->width = saved_width; gport->height = saved_height;
  PCB->MaxWidth = saved_max_width; PCB->MaxHeight = saved_max_height;
  cur_mask = saved_mode; priv->clip = saved_clip; set_clip (priv, priv->bg_gc);
  return FALSE;
}''',
}


FUNCTIONS['set_clip'] = r'''static void
set_clip (render_priv *priv, PcbCairoGC *gc)
{
  if (gc) pcb_cairo_gc_set_clip_rectangle (gc, priv->clip ? &priv->clip_rect : NULL);
}'''
