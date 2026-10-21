/* Cairo image-surface primitives. XOR uses binary coverage and integer pixels,
 * preserving PCB's draw-twice-to-erase contract without antialiasing trails. */
#include "pcb-cairo.h"
#include <math.h>
#include <stdint.h>

struct PcbCairoGC
{
  GdkRGBA color;
  PcbCairoOperation operation;
  int width;
  cairo_line_cap_t cap;
  cairo_line_join_t join;
  gboolean clipped;
  GdkRectangle clip;
  cairo_surface_t *mask;
};

/*! Allocate independent drawing state; surfaces are supplied per operation. */
PcbCairoGC *
pcb_cairo_gc_new (cairo_surface_t *surface)
{
  PcbCairoGC *gc = g_new0 (PcbCairoGC, 1);
  (void) surface;
  gc->color.alpha = 1;
  gc->width = 1;
  return gc;
}

/*! Release the optional mask reference and graphics state. */
void
pcb_cairo_gc_free (PcbCairoGC *gc)
{
  if (!gc) return;
  if (gc->mask) cairo_surface_destroy (gc->mask);
  g_free (gc);
}

/*! Copy value-type RGBA color into the graphics state. */
void
pcb_cairo_gc_set_foreground (PcbCairoGC *gc, const GdkRGBA *color)
{ gc->color = *color; }

/*! Select opaque copy or the legacy reversible RGB operation. */
void
pcb_cairo_gc_set_function (PcbCairoGC *gc, PcbCairoOperation operation)
{ gc->operation = operation; }

/*! Store device-pixel line attributes; zero widths remain one-pixel hairlines. */
void
pcb_cairo_gc_set_line_attributes (PcbCairoGC *gc, int width,
                                  cairo_line_cap_t cap, cairo_line_join_t join)
{ gc->width = MAX (1, width); gc->cap = cap; gc->join = join; }

/*! Copy a clip rectangle, or disable rectangular clipping. */
void
pcb_cairo_gc_set_clip_rectangle (PcbCairoGC *gc, const GdkRectangle *rect)
{ gc->clipped = rect != NULL; if (rect) gc->clip = *rect; }

/*! Retain a clearance mask independently of the port's surface ownership. */
void
pcb_cairo_gc_set_clip_mask (PcbCairoGC *gc, cairo_surface_t *mask)
{
  if (mask) cairo_surface_reference (mask);
  if (gc->mask) cairo_surface_destroy (gc->mask);
  gc->mask = mask;
}

/*! Convert normalized channels to the eight-bit raster representation. */
static unsigned int
channel (double value)
{ return (unsigned int) (CLAMP (value, 0.0, 1.0) * 255.0 + 0.5); }

/*! Construct the RGB XOR pen that displays color a against background b. */
void
pcb_cairo_color_xor (GdkRGBA *result, const GdkRGBA *a, const GdkRGBA *b)
{
  result->red = (channel (a->red) ^ channel (b->red)) / 255.0;
  result->green = (channel (a->green) ^ channel (b->green)) / 255.0;
  result->blue = (channel (a->blue) ^ channel (b->blue)) / 255.0;
  result->alpha = 1;
}

/*! Start one primitive in a temporary image to support masking and exact XOR. */
static cairo_t *
begin (cairo_surface_t *surface, PcbCairoGC *gc, cairo_surface_t **paint)
{
  cairo_t *cr;
  *paint = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
  cr = cairo_create (*paint);
  cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
  if (gc->clipped)
    {
      cairo_rectangle (cr, gc->clip.x, gc->clip.y, gc->clip.width, gc->clip.height);
      cairo_clip (cr);
    }
  cairo_set_source_rgba (cr, gc->color.red, gc->color.green,
                         gc->color.blue, cairo_image_surface_get_format (surface) == CAIRO_FORMAT_A8
                          ? 1.0 : gc->color.alpha);
  cairo_set_line_width (cr, gc->width);
  cairo_set_line_cap (cr, gc->cap);
  cairo_set_line_join (cr, gc->join);
  return cr;
}

/*! Commit binary coverage, preserving alpha and applying the clearance mask.
 * A8 masks use the red channel: white permits subsequent paint, black clears.
 */
static void
finish (cairo_surface_t *surface, PcbCairoGC *gc, cairo_t *cr, cairo_surface_t *paint)
{
  double bx, by, bw, bh;
  int x, y, x0, y0, width, height;
  int dst_stride = cairo_image_surface_get_stride (surface);
  int src_stride;
  unsigned char *dst, *src, *mask = NULL;
  int mask_stride = 0;
  cairo_surface_t *raster;
  cairo_t *copy;
  cairo_format_t format = cairo_image_surface_get_format (surface);
  cairo_destroy (cr);
  cairo_recording_surface_ink_extents (paint, &bx, &by, &bw, &bh);
  x0 = MAX (0, (int) floor (bx)); y0 = MAX (0, (int) floor (by));
  width = MIN (cairo_image_surface_get_width (surface), (int) ceil (bx + bw)) - x0;
  height = MIN (cairo_image_surface_get_height (surface), (int) ceil (by + bh)) - y0;
  if (width <= 0 || height <= 0) { cairo_surface_destroy (paint); return; }
  raster = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  copy = cairo_create (raster);
  cairo_set_source_surface (copy, paint, -x0, -y0); cairo_paint (copy); cairo_destroy (copy);
  cairo_surface_flush (surface); cairo_surface_flush (raster);
  dst = cairo_image_surface_get_data (surface); src = cairo_image_surface_get_data (raster);
  src_stride = cairo_image_surface_get_stride (raster);
  if (gc->mask)
    {
      g_assert (cairo_image_surface_get_width (gc->mask) == cairo_image_surface_get_width (surface));
      g_assert (cairo_image_surface_get_height (gc->mask) == cairo_image_surface_get_height (surface));
      cairo_surface_flush (gc->mask);
      mask = cairo_image_surface_get_data (gc->mask);
      mask_stride = cairo_image_surface_get_stride (gc->mask);
    }
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
        uint32_t pixel = ((uint32_t *) (src + y * src_stride))[x];
        if ((pixel >> 24) == 0 || (mask && mask[(y + y0) * mask_stride + x + x0] == 0)) continue;
        if (format == CAIRO_FORMAT_A8)
          dst[(y + y0) * dst_stride + x + x0] = channel (gc->color.red);
        else
          {
            uint32_t *target = &((uint32_t *) (dst + (y + y0) * dst_stride))[x + x0];
            if (gc->operation == PCB_CAIRO_XOR)
              *target = (*target ^ (pixel & 0x00ffffffU)) | 0xff000000U;
            else
              {
                unsigned int a = pixel >> 24, inverse = 255 - a;
                unsigned int r = ((pixel >> 16) & 255) + (((*target >> 16) & 255) * inverse + 127) / 255;
                unsigned int g = ((pixel >> 8) & 255) + (((*target >> 8) & 255) * inverse + 127) / 255;
                unsigned int b = (pixel & 255) + ((*target & 255) * inverse + 127) / 255;
                unsigned int alpha = a + ((*target >> 24) * inverse + 127) / 255;
                *target = (alpha << 24) | (r << 16) | (g << 8) | b;
              }
          }
      }
  cairo_surface_mark_dirty (surface);
  cairo_surface_destroy (raster);
  cairo_surface_destroy (paint);
}

/*! Stroke a line in device pixels, placing hairlines on pixel centers. */
void
pcb_cairo_draw_line (cairo_surface_t *surface, PcbCairoGC *gc,
                      double x1, double y1, double x2, double y2)
{
  cairo_surface_t *paint; cairo_t *cr = begin (surface, gc, &paint);
  cairo_move_to (cr, x1 + 0.5, y1 + 0.5);
  cairo_line_to (cr, x2 + 0.5, y2 + 0.5);
  cairo_stroke (cr); finish (surface, gc, cr, paint);
}

/*! Fill or stroke a device rectangle. */
void
pcb_cairo_draw_rectangle (cairo_surface_t *surface, PcbCairoGC *gc,
                           int fill, double x, double y, double w, double h)
{
  cairo_surface_t *paint; cairo_t *cr = begin (surface, gc, &paint);
  cairo_rectangle (cr, x, y, w, h);
  if (fill) cairo_fill (cr); else cairo_stroke (cr);
  finish (surface, gc, cr, paint);
}

/*! Draw an ellipse arc, retaining PCB/GDK's counterclockwise 1/64 degree units. */
void
pcb_cairo_draw_arc (cairo_surface_t *surface, PcbCairoGC *gc, int fill,
                     double x, double y, double w, double h, int start, int delta)
{
  cairo_surface_t *paint; cairo_t *cr;
  double angle = start * G_PI / (180.0 * 64.0);
  double sweep = delta * G_PI / (180.0 * 64.0);
  if (w <= 0 || h <= 0 || delta == 0) return;
  cr = begin (surface, gc, &paint);
  cairo_save (cr);
  cairo_translate (cr, x + w / 2.0, y + h / 2.0);
  cairo_scale (cr, w / 2.0, h / 2.0);
  if (fill) cairo_move_to (cr, 0, 0);
  if (delta > 0) cairo_arc_negative (cr, 0, 0, 1, -angle, -angle - sweep);
  else cairo_arc (cr, 0, 0, 1, -angle, -angle - sweep);
  if (fill) cairo_close_path (cr);
  cairo_restore (cr);
  if (fill) cairo_fill (cr); else cairo_stroke (cr);
  finish (surface, gc, cr, paint);
}

/*! Fill or stroke a closed polygon without mutating the input points. */
void
pcb_cairo_draw_polygon (cairo_surface_t *surface, PcbCairoGC *gc, int fill,
                         const PcbCairoPoint *points, int count)
{
  cairo_surface_t *paint; cairo_t *cr; int i;
  if (count < 3) return;
  cr = begin (surface, gc, &paint);
  cairo_move_to (cr, points[0].x, points[0].y);
  for (i = 1; i < count; i++) cairo_line_to (cr, points[i].x, points[i].y);
  cairo_close_path (cr);
  if (fill) cairo_fill (cr); else cairo_stroke (cr);
  finish (surface, gc, cr, paint);
}

/*! Copy a pixbuf region through Cairo rather than a removed GDK drawable. */
void
pcb_cairo_draw_pixbuf (cairo_surface_t *surface, PcbCairoGC *gc,
                        GdkPixbuf *pixbuf, int sx, int sy, int dx, int dy,
                        int width, int height)
{
  cairo_surface_t *paint; cairo_t *cr;
  if (width <= 0 || height <= 0) return;
  cr = begin (surface, gc, &paint);
  cairo_rectangle (cr, dx, dy, width, height); cairo_clip (cr);
  gdk_cairo_set_source_pixbuf (cr, pixbuf, dx - sx, dy - sy); cairo_paint (cr);
  finish (surface, gc, cr, paint);
}

/*! Preserve the original monochrome cursor artwork as a GTK3 RGBA pixbuf. */
GdkCursor *
pcb_cairo_cursor_from_bits (GdkDisplay *display, const unsigned char *bits,
                            const unsigned char *mask, int width, int height,
                            int hot_x, int hot_y)
{
  GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, width, height);
  GdkCursor *cursor;
  unsigned char *pixels = gdk_pixbuf_get_pixels (pixbuf);
  int x, y, stride = gdk_pixbuf_get_rowstride (pixbuf), bytes = (width + 7) / 8;
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
        unsigned char *pixel = pixels + y * stride + x * 4;
        unsigned char bit = 1U << (x % 8);
        pixel[0] = pixel[1] = pixel[2] = bits[y * bytes + x / 8] & bit ? 255 : 0;
        pixel[3] = mask[y * bytes + x / 8] & bit ? 255 : 0;
      }
  cursor = gdk_cursor_new_from_pixbuf (display, pixbuf, hot_x, hot_y);
  g_object_unref (pixbuf);
  return cursor;
}
