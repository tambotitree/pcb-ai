/* Shared GTK3 grid placement for converted dialog layouts. */
#ifndef PCB_GRID_H
#define PCB_GRID_H

#include <gtk/gtk.h>

/*! \brief Place a widget by cell boundaries, preserving expansion and padding. */
static inline void
pcb_grid_attach_bounds (GtkGrid *grid, GtkWidget *child,
                        guint left, guint right, guint top, guint bottom,
                        guint horizontal, guint vertical,
                        guint xpadding, guint ypadding)
{
  gtk_widget_set_hexpand (child, (horizontal & GTK_EXPAND) != 0);
  gtk_widget_set_vexpand (child, (vertical & GTK_EXPAND) != 0);
  gtk_widget_set_halign (child, (horizontal & GTK_FILL) ? GTK_ALIGN_FILL : GTK_ALIGN_CENTER);
  gtk_widget_set_valign (child, (vertical & GTK_FILL) ? GTK_ALIGN_FILL : GTK_ALIGN_CENTER);
  gtk_widget_set_margin_start (child, xpadding);
  gtk_widget_set_margin_end (child, xpadding);
  gtk_widget_set_margin_top (child, ypadding);
  gtk_widget_set_margin_bottom (child, ypadding);
  gtk_grid_attach (grid, child, left, top, right - left, bottom - top);
}

/*! \brief Place a widget with the default expanding, filling cell policy. */
static inline void
pcb_grid_attach_expanding (GtkGrid *grid, GtkWidget *child,
                           guint left, guint right, guint top, guint bottom)
{
  pcb_grid_attach_bounds (grid, child, left, right, top, bottom,
                          GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
}

#endif
