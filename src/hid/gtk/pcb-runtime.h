/* GTK runtime data discovery; GPL-2.0-or-later. */
#ifndef PCB_GTK_RUNTIME_H
#define PCB_GTK_RUNTIME_H

#include <glib.h>

/* Set the configure-discovered fallback before GTK/GSettings caches a source.
 * Respect explicit user configuration; do not change desktop settings.
 */
static inline void
pcb_gtk_runtime_init (void)
{
#ifdef PCB_GTK_SCHEMA_DIR
  gchar *compiled;
  if (g_getenv ("GSETTINGS_SCHEMA_DIR") != NULL || *PCB_GTK_SCHEMA_DIR == '\0')
    return;
  compiled = g_build_filename (PCB_GTK_SCHEMA_DIR, "gschemas.compiled", NULL);
  if (g_file_test (compiled, G_FILE_TEST_IS_REGULAR))
    g_setenv ("GSETTINGS_SCHEMA_DIR", PCB_GTK_SCHEMA_DIR, FALSE);
  g_free (compiled);
#endif
}

#endif
