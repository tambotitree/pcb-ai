/* Live-board adapter for the copied AI chat; GPL-2.0-or-later. */
#include <config.h>
#include "gui.h"
#include "file.h"

/* Serialize through PCB's native writer without SavePCB's filename/dirty-state
 * side effects. tmpfile supplies a private, automatically removed stream.
 * Caller owns the returned UTF-8 text; NULL signals a snapshot failure.
 */
gchar *
pcb_board_snapshot (void)
{
  FILE *stream;
  GString *text;
  gchar buffer[8192];
  gsize length;
  gboolean failed;
  if (PCB == NULL)
    return NULL;
  stream = tmpfile ();
  if (stream == NULL)
    return NULL;
  failed = pcb_write_board_snapshot (stream) != 0;
  if (fflush (stream) != 0 || fseek (stream, 0, SEEK_SET) != 0)
    failed = TRUE;
  text = g_string_new (NULL);
  while (!failed && (length = fread (buffer, 1, sizeof buffer, stream)) > 0)
    g_string_append_len (text, buffer, length);
  if (ferror (stream))
    failed = TRUE;
  if (fclose (stream) != 0)
    failed = TRUE;
  if (failed || !g_utf8_validate (text->str, text->len, NULL))
    {
      g_string_free (text, TRUE);
      return NULL;
    }
  return g_string_free (text, FALSE);
}
