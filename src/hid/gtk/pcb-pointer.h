/* GTK3 pointer-device access shared by the editor event handlers. */
#ifndef PCB_POINTER_H
#define PCB_POINTER_H

#include <gtk/gtk.h>

/*! \brief Obtain the display's borrowed default-seat pointer device. */
static inline GdkDevice *
pcb_pointer_device (GdkDisplay *display)
{
  GdkSeat *seat;
  if (!display) return NULL;
  seat = gdk_display_get_default_seat (display);
  return seat ? gdk_seat_get_pointer (seat) : NULL;
}

/*! \brief Query coordinates and modifiers relative to the drawing window. */
static inline void
pcb_pointer_query_window (GdkWindow *window, gint *x, gint *y,
                           GdkModifierType *modifiers)
{
  GdkDevice *device;
  if (x) *x = 0;
  if (y) *y = 0;
  if (modifiers) *modifiers = 0;
  if (!window) return;
  device = pcb_pointer_device (gdk_window_get_display (window));
  if (device) gdk_window_get_device_position (window, device, x, y, modifiers);
}

/*! \brief Query pointer coordinates in the display's screen coordinate space. */
static inline void
pcb_pointer_query_screen (GdkDisplay *display, GdkScreen **screen, gint *x, gint *y)
{
  GdkDevice *device = pcb_pointer_device (display);
  if (screen) *screen = NULL;
  if (x) *x = 0;
  if (y) *y = 0;
  if (device) gdk_device_get_position (device, screen, x, y);
}

/*! \brief Request pointer movement using the display's explicit pointer device. */
static inline void
pcb_pointer_warp (GdkDisplay *display, GdkScreen *screen, gint x, gint y)
{
  GdkDevice *device = pcb_pointer_device (display);
  if (device && screen) gdk_device_warp (device, screen, x, y);
}

#endif
