/*!
 * \file src/hid/gtk/gtkhid-gdk.c
 *
 * \brief Functions for drawing on the GTK 2D graphics canvas
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996,1997,1998,2005,2006 Thomas Nau
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include "crosshair.h"
#include "clip.h"
#include "../hidint.h"
#include "gui.h"
#include "pcb-cairo.h"
#include "hid/common/draw_helpers.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif

extern HID ghid_hid;
extern HID_DRAW ghid_graphics;

/* Sets priv->u_gc to the "right" GC to use (wrt mask or window)
*/
#define USE_GC(gc) if (!use_gc(gc)) return

static enum mask_mode cur_mask = HID_MASK_OFF;


typedef struct render_priv {
  PcbCairoGC *bg_gc;
  PcbCairoGC *offlimits_gc;
  PcbCairoGC *mask_gc;
  PcbCairoGC *u_gc;
  PcbCairoGC *grid_gc;
  bool clip;
  GdkRectangle clip_rect;
  int attached_invalidate_depth;
  int mark_invalidate_depth;

  /* Feature for leading the user to a particular location */
  guint lead_user_timeout;
  GTimer *lead_user_timer;
  bool lead_user;
  Coord lead_user_radius;
  Coord lead_user_x;
  Coord lead_user_y;

  hidGC crosshair_gc;
} render_priv;


typedef struct hid_gc_struct
{
  HID *me_pointer;
  PcbCairoGC *gc;

  gchar *colorname;
  Coord width;
  gint cap, join;
  gchar xor_mask;
  gint mask_seq;
}
hid_gc_struct;


static void draw_lead_user (render_priv *priv);
static void set_special_grid_color (void);


int
ghid_set_layer (const char *name, int group, int empty)
{
  int idx = group;
  /* This renderer selects layers by group, not by the optional HID hints. */
  (void) name;
  (void) empty;
  if (idx >= 0 && idx < max_group)
    {
      int n = PCB->LayerGroups.Number[group];
      for (idx = 0; idx < n-1; idx ++)
	{
	  int ni = PCB->LayerGroups.Entries[group][idx];
	  if (ni >= 0 && ni < max_copper_layer + SILK_LAYER
	      && PCB->Data->Layer[ni].On)
	    break;
	}
      idx = PCB->LayerGroups.Entries[group][idx];
    }

  if (idx >= 0 && idx < max_copper_layer + SILK_LAYER)
    return /*pinout ? 1 : */ PCB->Data->Layer[idx].On;
  if (idx < 0)
    {
      switch (SL_TYPE (idx))
	{
	case SL_INVISIBLE:
	  return /* pinout ? 0 : */ PCB->InvisibleObjectsOn;
	case SL_MASK:
	  if (SL_MYSIDE (idx) /*&& !pinout */ )
	    return TEST_FLAG (SHOWMASKFLAG, PCB);
	  return 0;
	case SL_SILK:
	  if (SL_MYSIDE (idx) /*|| pinout */ )
	    return PCB->ElementOn;
	  return 0;
	case SL_ASSY:
	  return 0;
	case SL_PDRILL:
	case SL_UDRILL:
	  return 1;
	case SL_RATS:
	  return PCB->RatOn;
	}
    }
  return 0;
}

void
ghid_destroy_gc (hidGC gc)
{
  if (gc->gc)
    pcb_cairo_gc_free (gc->gc);
  g_free (gc->colorname);
  g_free (gc);
}

hidGC
ghid_make_gc (void)
{
  hidGC rv;

  rv = g_new0 (hid_gc_struct, 1);
  rv->me_pointer = &ghid_hid;
  rv->colorname = g_strdup (Settings.BackgroundColor);
  return rv;
}

/*!
 * \brief Set the drawing clip region.
 *
 * The clip region restricts drawing. Anything outside the clip region cannot
 * be changed.
 */
static void
set_clip (render_priv *priv, PcbCairoGC *gc)
{
  if (gc) pcb_cairo_gc_set_clip_rectangle (gc, priv->clip ? &priv->clip_rect : NULL);
}

/*!
 * \brief Draw the grid on the 2D canvas
 */
void
ghid_draw_grid (BoxType * region)
{
  Coord x1, y1, x2, y2, x, y;
  double point_size;
  unsigned int arc_size;
  render_priv *priv = gport->render_priv;

  /* The Cairo clip already restricts the visible grid. */
  (void) region;

  if (!Settings.DrawGrid)
    return; /* grid hidden */
  if (Vz (PCB->Grid) < MIN_GRID_DISTANCE)
    return; /* zoomed out too far, points to close together */
  if (!priv->grid_gc) /* create a graphics context if we don't have one */
  {
    set_special_grid_color ();
    priv->grid_gc = pcb_cairo_gc_new (gport->drawable);
    pcb_cairo_gc_set_function (priv->grid_gc, PCB_CAIRO_XOR);
    pcb_cairo_gc_set_foreground (priv->grid_gc, &gport->grid_color);

    set_clip (priv, priv->grid_gc);
  } /* end if (!priv->grid_gc) */
  x1 = GridFit (SIDE_X (gport->view.x0), PCB->Grid, PCB->GridOffsetX);
  y1 = GridFit (SIDE_Y (gport->view.y0), PCB->Grid, PCB->GridOffsetY);
  x2 = GridFit (SIDE_X (gport->view.x0 + gport->view.width - 1),
                PCB->Grid, PCB->GridOffsetX);
  y2 = GridFit (SIDE_Y (gport->view.y0 + gport->view.height - 1),
                PCB->Grid, PCB->GridOffsetY);
  if (x1 > x2)
    {
      Coord tmp = x1;
      x1 = x2;
      x2 = tmp;
    }
  if (y1 > y2)
    {
      Coord tmp = y1;
      y1 = y2;
      y2 = tmp;
    }

  /* The bounding points could have been outside of the drawing area */
  if (Vx (x1) < 0)    x1 += PCB->Grid;
  if (Vy (y1) < 0)    y1 += PCB->Grid;
  if (Vx (x2) >= gport->width)    x2 -= PCB->Grid;
  if (Vy (y2) >= gport->height)    y2 -= PCB->Grid;

  point_size = 1.0 + Vz (PCB->Grid) / 30.0;
  /* limit the maximum and minimum size of the grid dot - these should really be taken from settings */
  if (point_size > 4)
    {
      point_size = 4;
    }
  else if (point_size < 1)
    {
      point_size = 1;
    }

  /* ensure grid point size is an integer value to prevent moira artefacts */
  arc_size = (int)(point_size + 0.5);

  for (x = x1; x <= x2; x += PCB->Grid)
    for (y = y1; y <= y2; y += PCB->Grid)
      pcb_cairo_draw_arc (gport->drawable, priv->grid_gc,
                    1,
                    Vx (x) - arc_size / 2, Vy (y) - arc_size / 2,
                    arc_size, arc_size,
                    0, 360 * 64);

}

/* ------------------------------------------------------------ */
static void
ghid_draw_bg_image (void)
{
  static GdkPixbuf *pixbuf;
  GdkInterpType interp_type;
  gint x, y, w, h, w_src, h_src;
  static gint w_scaled, h_scaled;
  render_priv *priv = gport->render_priv;

  if (!ghidgui->bg_pixbuf)
    return;

  w = PCB->MaxWidth / gport->view.coord_per_px;
  h = PCB->MaxHeight / gport->view.coord_per_px;
  x = gport->view.x0 / gport->view.coord_per_px;
  y = gport->view.y0 / gport->view.coord_per_px;

  if (w_scaled != w || h_scaled != h)
    {
      if (pixbuf)
	g_object_unref (G_OBJECT (pixbuf));

      w_src = gdk_pixbuf_get_width (ghidgui->bg_pixbuf);
      h_src = gdk_pixbuf_get_height (ghidgui->bg_pixbuf);
      if (w > w_src && h > h_src)
	interp_type = GDK_INTERP_NEAREST;
      else
	interp_type = GDK_INTERP_BILINEAR;

      pixbuf =
	gdk_pixbuf_scale_simple (ghidgui->bg_pixbuf, w, h, interp_type);
      w_scaled = w;
      h_scaled = h;
    }
  if (pixbuf)
    pcb_cairo_draw_pixbuf (gport->drawable, priv->bg_gc, pixbuf,
				   x, y, 0, 0,
				   w - x, h - y);
}

#define WHICH_GC(gc) (cur_mask == HID_MASK_CLEAR ? priv->mask_gc : (gc)->gc)

void
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
}


typedef struct
{
  int color_set;
  GdkRGBA color;
  int xor_set;
  GdkRGBA xor_color;
} ColorCache;


  /* Config helper functions for when the user changes color preferences.
     |  set_special colors used in the gtkhid.
   */
static void
set_special_grid_color (void)
{
  render_priv *priv = gport->render_priv;
  GdkRGBA color;
  if (!gdk_rgba_parse (&color, Settings.GridColor)) color = (GdkRGBA) {1, 1, 1, 1};
  pcb_cairo_color_xor (&gport->grid_color, &color, &gport->bg_color);
  if (priv->grid_gc) pcb_cairo_gc_set_foreground (priv->grid_gc, &gport->grid_color);
}

void
ghid_set_special_colors (HID_Attribute * ha)
{
  render_priv *priv = gport->render_priv;

  if (!ha->name || !ha->value)
    return;
  if (!strcmp (ha->name, "background-color") && priv->bg_gc)
    {
      ghid_map_color_string (*(char **) ha->value, &gport->bg_color);
      pcb_cairo_gc_set_foreground (priv->bg_gc, &gport->bg_color);
      set_special_grid_color ();
    }
  else if (!strcmp (ha->name, "off-limit-color") && priv->offlimits_gc)
    {
      ghid_map_color_string (*(char **) ha->value, &gport->offlimits_color);
      pcb_cairo_gc_set_foreground (priv->offlimits_gc, &gport->offlimits_color);
    }
  else if (!strcmp (ha->name, "grid-color") && priv->grid_gc)
    {
      ghid_map_color_string (*(char **) ha->value, &gport->grid_color);
      set_special_grid_color ();
    }
}

void
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
}

void
ghid_set_line_cap (hidGC gc, EndCapStyle style)
{
  render_priv *priv = gport->render_priv;

  switch (style)
    {
    case Trace_Cap:
    case Round_Cap:
      gc->cap = CAIRO_LINE_CAP_ROUND;
      gc->join = CAIRO_LINE_JOIN_ROUND;
      break;
    case Square_Cap:
    case Beveled_Cap:
      gc->cap = CAIRO_LINE_CAP_SQUARE;
      gc->join = CAIRO_LINE_JOIN_MITER;
      break;
    }
  if (gc->gc)
    pcb_cairo_gc_set_line_attributes (WHICH_GC (gc),
				Vz (gc->width),
				(cairo_line_cap_t)gc->cap, (cairo_line_join_t)gc->join);
}

void
ghid_set_line_width (hidGC gc, Coord width)
{
  render_priv *priv = gport->render_priv;

  gc->width = width;
  if (gc->gc)
    pcb_cairo_gc_set_line_attributes (WHICH_GC (gc),
				Vz (gc->width),
				(cairo_line_cap_t)gc->cap, (cairo_line_join_t)gc->join);
}

void
ghid_set_draw_xor (hidGC gc, int xor_mask)
{
  gc->xor_mask = xor_mask;
  if (!gc->gc)
    return;
  pcb_cairo_gc_set_function (gc->gc, xor_mask ? PCB_CAIRO_XOR : PCB_CAIRO_COPY);
  ghid_set_color (gc, gc->colorname);
}

static int
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
}

void
ghid_draw_line (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  double dx1, dy1, dx2, dy2;
  render_priv *priv = gport->render_priv;

  dx1 = Vx ((double) x1);
  dy1 = Vy ((double) y1);
  dx2 = Vx ((double) x2);
  dy2 = Vy ((double) y2);

  if (!ClipLine (0, 0, gport->width, gport->height,
		 &dx1, &dy1, &dx2, &dy2, gc->width / gport->view.coord_per_px))
    return;

  USE_GC (gc);
  pcb_cairo_draw_line (gport->drawable, priv->u_gc, dx1, dy1, dx2, dy2);
}

void
ghid_draw_arc (hidGC gc, Coord cx, Coord cy,
	       Coord xradius, Coord yradius, Angle start_angle, Angle delta_angle)
{
  gint vrx, vry;
  gint w, h, radius;
  render_priv *priv = gport->render_priv;

  w = gport->width * gport->view.coord_per_px;
  h = gport->height * gport->view.coord_per_px;
  radius = (xradius > yradius) ? xradius : yradius;
  if (SIDE_X (cx) < gport->view.x0 - radius
      || SIDE_X (cx) > gport->view.x0 + w + radius
      || SIDE_Y (cy) < gport->view.y0 - radius
      || SIDE_Y (cy) > gport->view.y0 + h + radius)
    return;

  USE_GC (gc);
  vrx = Vz (xradius);
  vry = Vz (yradius);

  if (gport->view.flip_x)
    {
      start_angle = 180 - start_angle;
      delta_angle = -delta_angle;
    }
  if (gport->view.flip_y)
    {
      start_angle = -start_angle;
      delta_angle = -delta_angle;
    }
  /* make sure we fall in the -180 to +180 range */
  start_angle = NormalizeAngle (start_angle);
  if (start_angle >= 180)  start_angle -= 360;

  pcb_cairo_draw_arc (gport->drawable, priv->u_gc, 0,
		Vx (cx) - vrx, Vy (cy) - vry,
		vrx * 2, vry * 2, (start_angle + 180) * 64, delta_angle * 64);
}

void
ghid_draw_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  gint w, h, lw;
  render_priv *priv = gport->render_priv;

  lw = gc->width;
  w = gport->width * gport->view.coord_per_px;
  h = gport->height * gport->view.coord_per_px;

  if ((SIDE_X (x1) < gport->view.x0 - lw
       && SIDE_X (x2) < gport->view.x0 - lw)
      || (SIDE_X (x1) > gport->view.x0 + w + lw
	  && SIDE_X (x2) > gport->view.x0 + w + lw)
      || (SIDE_Y (y1) < gport->view.y0 - lw
	  && SIDE_Y (y2) < gport->view.y0 - lw)
      || (SIDE_Y (y1) > gport->view.y0 + h + lw
	  && SIDE_Y (y2) > gport->view.y0 + h + lw))
    return;

  x1 = Vx (x1);
  y1 = Vy (y1);
  x2 = Vx (x2);
  y2 = Vy (y2);

  if (x1 > x2)
    {
      gint xt = x1;
      x1 = x2;
      x2 = xt;
    }
  if (y1 > y2)
    {
      gint yt = y1;
      y1 = y2;
      y2 = yt;
    }

  USE_GC (gc);
  pcb_cairo_draw_rectangle (gport->drawable, priv->u_gc, FALSE,
		      x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}


void
ghid_fill_circle (hidGC gc, Coord cx, Coord cy, Coord radius)
{
  gint w, h, vr;
  render_priv *priv = gport->render_priv;

  w = gport->width * gport->view.coord_per_px;
  h = gport->height * gport->view.coord_per_px;
  if (SIDE_X (cx) < gport->view.x0 - radius
      || SIDE_X (cx) > gport->view.x0 + w + radius
      || SIDE_Y (cy) < gport->view.y0 - radius
      || SIDE_Y (cy) > gport->view.y0 + h + radius)
    return;

  USE_GC (gc);
  vr = Vz (radius);
  pcb_cairo_draw_arc (gport->drawable, priv->u_gc, TRUE,
		Vx (cx) - vr, Vy (cy) - vr, vr * 2, vr * 2, 0, 360 * 64);
}

void
ghid_fill_polygon (hidGC gc, int n_coords, Coord *x, Coord *y)
{
  static PcbCairoPoint *points = 0;
  static int npoints = 0;
  int i;
  render_priv *priv = gport->render_priv;
  USE_GC (gc);

  if (npoints < n_coords)
    {
      npoints = n_coords + 1;
      points = (PcbCairoPoint *)realloc (points, npoints * sizeof (PcbCairoPoint));
    }
  for (i = 0; i < n_coords; i++)
    {
      points[i].x = Vx (x[i]);
      points[i].y = Vy (y[i]);
    }
  pcb_cairo_draw_polygon (gport->drawable, priv->u_gc, 1, points, n_coords);
}

void
ghid_fill_rect (hidGC gc, Coord x1, Coord y1, Coord x2, Coord y2)
{
  gint w, h, lw, xx, yy;
  render_priv *priv = gport->render_priv;

  lw = gc->width;
  w = gport->width * gport->view.coord_per_px;
  h = gport->height * gport->view.coord_per_px;

  if ((SIDE_X (x1) < gport->view.x0 - lw
       && SIDE_X (x2) < gport->view.x0 - lw)
      || (SIDE_X (x1) > gport->view.x0 + w + lw
	  && SIDE_X (x2) > gport->view.x0 + w + lw)
      || (SIDE_Y (y1) < gport->view.y0 - lw
	  && SIDE_Y (y2) < gport->view.y0 - lw)
      || (SIDE_Y (y1) > gport->view.y0 + h + lw
	  && SIDE_Y (y2) > gport->view.y0 + h + lw))
    return;

  x1 = Vx (x1);
  y1 = Vy (y1);
  x2 = Vx (x2);
  y2 = Vy (y2);
  if (x2 < x1)
    {
      xx = x1;
      x1 = x2;
      x2 = xx;
    }
  if (y2 < y1)
    {
      yy = y1;
      y1 = y2;
      y2 = yy;
    }
  USE_GC (gc);
  pcb_cairo_draw_rectangle (gport->drawable, priv->u_gc, TRUE,
		      x1, y1, x2 - x1 + 1, y2 - y1 + 1);
}

/*!
 * \brief Redraw a region of the pcb workspace
 */
static void
redraw_region (GdkRectangle *rect)
{
  int eleft, eright, etop, ebottom;
  BoxType region; /* section to draw in PCB coordinates */
  render_priv *priv = gport->render_priv;

  if (!gport->pixmap)
    return;

  if (rect != NULL)
    { /* draw the region passed as an argument */
      priv->clip_rect = *rect;
      priv->clip = true;
    }
  else
    { /* specified region was null, draw the entire area */
      priv->clip_rect.x = 0;
      priv->clip_rect.y = 0;
      priv->clip_rect.width = gport->width;
      priv->clip_rect.height = gport->height;
      priv->clip = false;
    }

  /* set the clip to prevent changes to anything outside the region */
  set_clip (priv, priv->bg_gc);
  set_clip (priv, priv->offlimits_gc);
  set_clip (priv, priv->mask_gc);
  set_clip (priv, priv->grid_gc);

  /* Compute the PCB coordinates of the area to redraw */
  /* Find the upper and lower corners of the drawing area */
  region.X1 = MIN(Px(priv->clip_rect.x),
                  Px(priv->clip_rect.x + priv->clip_rect.width + 1));
  region.Y1 = MIN(Py(priv->clip_rect.y),
                  Py(priv->clip_rect.y + priv->clip_rect.height + 1));
  region.X2 = MAX(Px(priv->clip_rect.x),
                  Px(priv->clip_rect.x + priv->clip_rect.width + 1));
  region.Y2 = MAX(Py(priv->clip_rect.y),
                  Py(priv->clip_rect.y + priv->clip_rect.height + 1));

  /* Restrict the drawing region to inside the PCB area */
  region.X1 = MAX (0, MIN (PCB->MaxWidth,  region.X1));
  region.X2 = MAX (0, MIN (PCB->MaxWidth,  region.X2));
  region.Y1 = MAX (0, MIN (PCB->MaxHeight, region.Y1));
  region.Y2 = MAX (0, MIN (PCB->MaxHeight, region.Y2));

  /* Compute the viewport coordinates of the edges of the PCB area */
  eleft = Vx (0);
  eright = Vx (PCB->MaxWidth);
  etop = Vy (0);
  ebottom = Vy (PCB->MaxHeight);
  if (eleft > eright)
    {
      int tmp = eleft;
      eleft = eright;
      eright = tmp;
    }
  if (etop > ebottom)
    {
      int tmp = etop;
      etop = ebottom;
      ebottom = tmp;
    }

  /* If the PCB isn't filling the entire screen, draw the dead area around it */
  if (eleft > 0) /* draw dead area on the left side */
    pcb_cairo_draw_rectangle (gport->drawable, priv->offlimits_gc,
                        1, 0, 0, eleft, gport->height);
  else
    eleft = 0;
  if (eright < gport->width) /* draw dead area on the right side */
    pcb_cairo_draw_rectangle (gport->drawable, priv->offlimits_gc,
                        1, eright, 0, gport->width - eright, gport->height);
  else
    eright = gport->width;
  if (etop > 0) /* draw dead area on the top */
    pcb_cairo_draw_rectangle (gport->drawable, priv->offlimits_gc,
                        1, eleft, 0, eright - eleft + 1, etop);
  else
    etop = 0;
  if (ebottom < gport->height) /* draw dead area on the bottom */
    pcb_cairo_draw_rectangle (gport->drawable, priv->offlimits_gc,
                        1, eleft, ebottom, eright - eleft + 1,
                        gport->height - ebottom);
  else
    ebottom = gport->height;

  /* Draw the PCB background color */
  pcb_cairo_draw_rectangle (gport->drawable, priv->bg_gc, 1,
                      eleft, etop, eright - eleft + 1, ebottom - etop + 1);

  ghid_draw_bg_image();

  /* Draw all of the PCB stuff, elements, traces, etc. */
  hid_expose_callback (&ghid_hid, &region, 0);

  ghid_graphics.draw_grid (&region);

  /* In some cases we are called with the crosshair still off */
  if (priv->attached_invalidate_depth == 0)
    DrawAttached (priv->crosshair_gc);

  /* In some cases we are called with the mark still off */
  if (priv->mark_invalidate_depth == 0)
    DrawMark (priv->crosshair_gc);

  draw_lead_user (priv);

  /* Turn clipping off */
  priv->clip = false;

  /* Reset the clip for bg_gc, as it is used outside this function */
  pcb_cairo_gc_set_clip_mask (priv->bg_gc, NULL);
}

void
ghid_invalidate_lr (Coord left, Coord right, Coord top, Coord bottom)
{
  Coord dleft, dright, dtop, dbottom;
  Coord minx, maxx, miny, maxy;
  GdkRectangle rect;

  dleft = Vx (left);
  dright = Vx (right);
  dtop = Vy (top);
  dbottom = Vy (bottom);

  minx = MIN (dleft, dright);
  maxx = MAX (dleft, dright);
  miny = MIN (dtop, dbottom);
  maxy = MAX (dtop, dbottom);

  rect.x = minx;
  rect.y = miny;
  rect.width = maxx - minx;
  rect.height = maxy - miny;

  redraw_region (&rect);
  ghid_screen_update ();
}


void
ghid_invalidate_all (void)
{
  redraw_region (NULL);
  ghid_screen_update ();
}

void
ghid_notify_crosshair_change (bool changes_complete)
{
  render_priv *priv = gport->render_priv;

  /* We sometimes get called before the GUI is up */
  if (gport->drawing_area == NULL)
    return;

  if (changes_complete)
    priv->attached_invalidate_depth --;

  if (priv->attached_invalidate_depth < 0)
    {
      priv->attached_invalidate_depth = 0;
      /* A mismatch of changes_complete == false and == true notifications
       * is not expected to occur, but we will try to handle it gracefully.
       * As we know the crosshair will have been shown already, we must
       * repaint the entire view to be sure not to leave an artaefact.
       */
      ghid_invalidate_all ();
      return;
    }

  if (priv->attached_invalidate_depth == 0)
    DrawAttached (priv->crosshair_gc);

  if (!changes_complete)
    {
      priv->attached_invalidate_depth ++;
    }
  else if (gport->drawing_area != NULL)
    {
      /* Queue a GTK expose when changes are complete */
      ghid_draw_area_update (gport, NULL);
    }
}

void
ghid_notify_mark_change (bool changes_complete)
{
  render_priv *priv = gport->render_priv;

  /* We sometimes get called before the GUI is up */
  if (gport->drawing_area == NULL)
    return;

  if (changes_complete)
    priv->mark_invalidate_depth --;

  if (priv->mark_invalidate_depth < 0)
    {
      priv->mark_invalidate_depth = 0;
      /* A mismatch of changes_complete == false and == true notifications
       * is not expected to occur, but we will try to handle it gracefully.
       * As we know the mark will have been shown already, we must
       * repaint the entire view to be sure not to leave an artaefact.
       */
      ghid_invalidate_all ();
      return;
    }

  if (priv->mark_invalidate_depth == 0)
    DrawMark (priv->crosshair_gc);

  if (!changes_complete)
    {
      priv->mark_invalidate_depth ++;
    }
  else if (gport->drawing_area != NULL)
    {
      /* Queue a GTK expose when changes are complete */
      ghid_draw_area_update (gport, NULL);
    }
}

static void
draw_right_cross (PcbCairoGC *xor_gc, gint x, gint y)
{
  cairo_surface_t *window = gport->drawable;

  pcb_cairo_draw_line (window, xor_gc, x, 0, x, gport->height);
  pcb_cairo_draw_line (window, xor_gc, 0, y, gport->width, y);
}

static void
draw_slanted_cross (PcbCairoGC *xor_gc, gint x, gint y)
{
  cairo_surface_t *window = gport->drawable;
  gint x0, y0, x1, y1;

  x0 = x + (gport->height - y);
  x0 = MAX(0, MIN (x0, gport->width));
  x1 = x - y;
  x1 = MAX(0, MIN (x1, gport->width));
  y0 = y + (gport->width - x);
  y0 = MAX(0, MIN (y0, gport->height));
  y1 = y - x;
  y1 = MAX(0, MIN (y1, gport->height));
  pcb_cairo_draw_line (window, xor_gc, x0, y0, x1, y1);

  x0 = x - (gport->height - y);
  x0 = MAX(0, MIN (x0, gport->width));
  x1 = x + y;
  x1 = MAX(0, MIN (x1, gport->width));
  y0 = y + x;
  y0 = MAX(0, MIN (y0, gport->height));
  y1 = y - (gport->width - x);
  y1 = MAX(0, MIN (y1, gport->height));
  pcb_cairo_draw_line (window, xor_gc, x0, y0, x1, y1);
}

static void
draw_dozen_cross (PcbCairoGC *xor_gc, gint x, gint y)
{
  cairo_surface_t *window = gport->drawable;
  gint x0, y0, x1, y1;
  gdouble tan60 = sqrt (3);

  x0 = x + (gport->height - y) / tan60;
  x0 = MAX(0, MIN (x0, gport->width));
  x1 = x - y / tan60;
  x1 = MAX(0, MIN (x1, gport->width));
  y0 = y + (gport->width - x) * tan60;
  y0 = MAX(0, MIN (y0, gport->height));
  y1 = y - x * tan60;
  y1 = MAX(0, MIN (y1, gport->height));
  pcb_cairo_draw_line (window, xor_gc, x0, y0, x1, y1);

  x0 = x + (gport->height - y) * tan60;
  x0 = MAX(0, MIN (x0, gport->width));
  x1 = x - y * tan60;
  x1 = MAX(0, MIN (x1, gport->width));
  y0 = y + (gport->width - x) / tan60;
  y0 = MAX(0, MIN (y0, gport->height));
  y1 = y - x / tan60;
  y1 = MAX(0, MIN (y1, gport->height));
  pcb_cairo_draw_line (window, xor_gc, x0, y0, x1, y1);

  x0 = x - (gport->height - y) / tan60;
  x0 = MAX(0, MIN (x0, gport->width));
  x1 = x + y / tan60;
  x1 = MAX(0, MIN (x1, gport->width));
  y0 = y + x * tan60;
  y0 = MAX(0, MIN (y0, gport->height));
  y1 = y - (gport->width - x) * tan60;
  y1 = MAX(0, MIN (y1, gport->height));
  pcb_cairo_draw_line (window, xor_gc, x0, y0, x1, y1);

  x0 = x - (gport->height - y) * tan60;
  x0 = MAX(0, MIN (x0, gport->width));
  x1 = x + y * tan60;
  x1 = MAX(0, MIN (x1, gport->width));
  y0 = y + x / tan60;
  y0 = MAX(0, MIN (y0, gport->height));
  y1 = y - (gport->width - x) / tan60;
  y1 = MAX(0, MIN (y1, gport->height));
  pcb_cairo_draw_line (window, xor_gc, x0, y0, x1, y1);
}

static void
draw_crosshair (render_priv *priv)
{
  cairo_surface_t *window = gport->drawable;

  gint x, y;
  static PcbCairoGC *xor_gc;
  static GdkRGBA cross_color;

  if (gport->crosshair_x < 0 || ghidgui->creating || !gport->has_entered)
    return;

  if (!xor_gc)
    {
      xor_gc = pcb_cairo_gc_new (window);

      pcb_cairo_gc_set_function (xor_gc, PCB_CAIRO_XOR);

      set_clip (priv, xor_gc);
      /* FIXME: when CrossColor changed from config */
      ghid_map_color_string (Settings.CrossColor, &cross_color);
    }

  pcb_cairo_gc_set_foreground (xor_gc, &cross_color);

  x = DRAW_X (gport->crosshair_x);
  y = DRAW_Y (gport->crosshair_y);

  draw_right_cross (xor_gc, x, y);
  if (Crosshair.shape == Union_Jack_Crosshair_Shape)
    draw_slanted_cross (xor_gc, x, y);
  if (Crosshair.shape == Dozen_Crosshair_Shape)
    draw_dozen_cross (xor_gc, x, y);
}

void
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
}

void
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
}

void
ghid_init_drawing_widget (GtkWidget *widget, GHidPort *port)
{
  (void) widget; (void) port;
}

void
ghid_drawing_area_configure_hook (GHidPort *port)
{
  render_priv *priv = port->render_priv;
  pcb_cairo_gc_set_foreground (priv->bg_gc, &port->bg_color);
  pcb_cairo_gc_set_foreground (priv->offlimits_gc, &port->offlimits_color);
  if (port->mask) { cairo_surface_destroy (port->mask); port->mask = NULL; }
  cur_mask = HID_MASK_OFF;
}

void
ghid_screen_update (void)
{
  if (gport->drawing_area) gtk_widget_queue_draw (gport->drawing_area);
}

gboolean
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
}

void
ghid_port_drawing_realize_cb (GtkWidget *widget, gpointer data)
{
  (void) widget; (void) data;
}

gboolean
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
}

cairo_surface_t *
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
}

HID_DRAW *
ghid_request_debug_draw (void)
{
  /* No special setup requirements, drawing goes into
   * the backing pixmap. */
  return ghid_hid.graphics;
}

void
ghid_flush_debug_draw (void)
{
  ghid_screen_update ();
  if (gport->drawing_area) gdk_display_flush (gtk_widget_get_display (gport->drawing_area));
}

void
ghid_finish_debug_draw (void)
{
  ghid_flush_debug_draw ();
  /* No special tear down requirements
   */
}

bool
ghid_event_to_pcb_coords (int event_x, int event_y, Coord *pcb_x, Coord *pcb_y)
{
  *pcb_x = EVENT_TO_PCB_X (event_x);
  *pcb_y = EVENT_TO_PCB_Y (event_y);

  return true;
}

bool
ghid_pcb_to_event_coords (Coord pcb_x, Coord pcb_y, int *event_x, int *event_y)
{
  *event_x = DRAW_X (pcb_x);
  *event_y = DRAW_Y (pcb_y);

  return true;
}


#define LEAD_USER_WIDTH           0.2          /* millimeters */
#define LEAD_USER_PERIOD          (1000 / 5)   /* 5fps (in ms) */
#define LEAD_USER_VELOCITY        3.           /* millimeters per second */
#define LEAD_USER_ARC_COUNT       3
#define LEAD_USER_ARC_SEPARATION  3.           /* millimeters */
#define LEAD_USER_INITIAL_RADIUS  10.          /* millimetres */
#define LEAD_USER_COLOR_R         1.
#define LEAD_USER_COLOR_G         1.
#define LEAD_USER_COLOR_B         0.

static void
draw_lead_user (render_priv *priv)
{
  cairo_surface_t *window = gport->drawable;

  int i;
  Coord radius = priv->lead_user_radius;
  Coord width = MM_TO_COORD (LEAD_USER_WIDTH);
  Coord separation = MM_TO_COORD (LEAD_USER_ARC_SEPARATION);
  static PcbCairoGC *lead_gc = NULL;
  GdkRGBA lead_color;

  if (!priv->lead_user)
    return;

  if (lead_gc == NULL)
    {
      lead_gc = pcb_cairo_gc_new (window);

      pcb_cairo_gc_set_function (lead_gc, PCB_CAIRO_XOR);

      lead_color.alpha = 1;
      lead_color.red = LEAD_USER_COLOR_R;
      lead_color.green = LEAD_USER_COLOR_G;
      lead_color.blue = LEAD_USER_COLOR_B;

      pcb_cairo_gc_set_foreground (lead_gc, &lead_color);
    }

  set_clip (priv, lead_gc);
  pcb_cairo_gc_set_line_attributes (lead_gc, Vz (width),
                               CAIRO_LINE_CAP_BUTT, CAIRO_LINE_JOIN_MITER);

  /* arcs at the approrpriate radii */

  for (i = 0; i < LEAD_USER_ARC_COUNT; i++, radius -= separation)
    {
      if (radius < width)
        radius += MM_TO_COORD (LEAD_USER_INITIAL_RADIUS);

      /* Draw an arc at radius */
      pcb_cairo_draw_arc (gport->drawable, lead_gc, FALSE,
                    Vx (priv->lead_user_x - radius),
                    Vy (priv->lead_user_y - radius),
                    Vz (2. * radius), Vz (2. * radius),
                    0, 360 * 64);
    }
}

static gboolean
lead_user_cb (gpointer data)
{
  render_priv *priv = data;
  Coord step;
  double elapsed_time;

  /* Queue a redraw */
  ghid_invalidate_all ();

  /* Update radius */
  elapsed_time = g_timer_elapsed (priv->lead_user_timer, NULL);
  g_timer_start (priv->lead_user_timer);

  step = MM_TO_COORD (LEAD_USER_VELOCITY * elapsed_time);
  if (priv->lead_user_radius > step)
    priv->lead_user_radius -= step;
  else
    priv->lead_user_radius = MM_TO_COORD (LEAD_USER_INITIAL_RADIUS);

  return TRUE;
}

void
ghid_lead_user_to_location (Coord x, Coord y)
{
  render_priv *priv = gport->render_priv;

  ghid_cancel_lead_user ();

  priv->lead_user = true;
  priv->lead_user_x = x;
  priv->lead_user_y = y;
  priv->lead_user_radius = MM_TO_COORD (LEAD_USER_INITIAL_RADIUS);
  priv->lead_user_timeout = g_timeout_add (LEAD_USER_PERIOD, lead_user_cb, priv);
  priv->lead_user_timer = g_timer_new ();
}

void
ghid_cancel_lead_user (void)
{
  render_priv *priv = gport->render_priv;

  if (priv->lead_user_timeout)
    g_source_remove (priv->lead_user_timeout);

  if (priv->lead_user_timer)
    g_timer_destroy (priv->lead_user_timer);

  priv->lead_user = false;
  if (gport->drawing_area) ghid_invalidate_all ();

  priv->lead_user_timeout = 0;
  priv->lead_user_timer = NULL;
  priv->lead_user = false;
}
