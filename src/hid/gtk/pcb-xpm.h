#ifndef PCB_GTK_XPM_H
#define PCB_GTK_XPM_H

#include <gtk/gtk.h>

/* Decode trusted compiled XPM rows using the supported stream API.
 * The stream owns its serialized buffer; the caller owns the returned pixbuf.
 */
static inline GdkPixbuf *
pcb_pixbuf_from_xpm (char *const *rows, gsize count)
{
  GString *document;
  GInputStream *stream;
  GdkPixbuf *pixbuf;
  GError *error = NULL;
  gsize i;
  gsize length;
  gchar *data;

  document = g_string_new ("/* XPM */\nstatic char *image[] = {\n");
  for (i = 0; i < count; i++)
    {
      gchar *escaped = g_strescape (rows[i], NULL);
      g_string_append_printf (document, "\"%s\"%s\n", escaped,
                              i + 1 < count ? "," : "");
      g_free (escaped);
    }
  g_string_append (document, "};\n");
  length = document->len;
  data = g_string_free (document, FALSE);
  stream = g_memory_input_stream_new_from_data (data, length, g_free);
  pixbuf = gdk_pixbuf_new_from_stream (stream, NULL, &error);
  g_object_unref (stream);
  if (pixbuf == NULL)
    {
      g_warning ("Cannot decode bundled PCB XPM image: %s", error->message);
      g_clear_error (&error);
    }
  return pixbuf;
}

#endif
