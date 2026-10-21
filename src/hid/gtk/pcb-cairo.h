/* Typed Cairo raster drawing state for PCB's GTK3 backend. */
#ifndef PCB_CAIRO_H
#define PCB_CAIRO_H

#include <gtk/gtk.h>

typedef struct PcbCairoGC PcbCairoGC;
typedef struct { int x, y; } PcbCairoPoint;
typedef enum { PCB_CAIRO_COPY, PCB_CAIRO_XOR } PcbCairoOperation;

PcbCairoGC *pcb_cairo_gc_new (cairo_surface_t *surface);
void pcb_cairo_gc_free (PcbCairoGC *gc);
void pcb_cairo_gc_set_foreground (PcbCairoGC *gc, const GdkRGBA *color);
void pcb_cairo_gc_set_function (PcbCairoGC *gc, PcbCairoOperation operation);
void pcb_cairo_gc_set_line_attributes (PcbCairoGC *gc, int width,
                                      cairo_line_cap_t cap, cairo_line_join_t join);
void pcb_cairo_gc_set_clip_rectangle (PcbCairoGC *gc, const GdkRectangle *rect);
void pcb_cairo_gc_set_clip_mask (PcbCairoGC *gc, cairo_surface_t *mask);
void pcb_cairo_color_xor (GdkRGBA *result, const GdkRGBA *a, const GdkRGBA *b);
void pcb_cairo_draw_line (cairo_surface_t *surface, PcbCairoGC *gc,
                          double x1, double y1, double x2, double y2);
void pcb_cairo_draw_rectangle (cairo_surface_t *surface, PcbCairoGC *gc,
                               int fill, double x, double y, double w, double h);
void pcb_cairo_draw_arc (cairo_surface_t *surface, PcbCairoGC *gc, int fill,
                         double x, double y, double w, double h, int start, int delta);
void pcb_cairo_draw_polygon (cairo_surface_t *surface, PcbCairoGC *gc, int fill,
                             const PcbCairoPoint *points, int count);
void pcb_cairo_draw_pixbuf (cairo_surface_t *surface, PcbCairoGC *gc,
                            GdkPixbuf *pixbuf, int sx, int sy, int dx, int dy,
                            int width, int height);
GdkCursor *pcb_cairo_cursor_from_bits (GdkDisplay *display,
                                       const unsigned char *bits,
                                       const unsigned char *mask,
                                       int width, int height, int hot_x, int hot_y);

#endif
