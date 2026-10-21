/* gEDA AI Assist JSON string encoding; GPL-2.0-or-later. */
#include <config.h>
#include "pcb_ai_chat.h"

/*! \brief Encode UTF-8 text as a JSON string; caller frees the result.
 * \param text Valid UTF-8 text (NULL means empty).
 * \return Allocated JSON string, including quotation marks.
 */
gchar *
pcb_ai_chat_quote (const gchar *text)
{
  GString *out = g_string_new ("\"");
  for (const guchar *p = (const guchar *) (text != NULL ? text : ""); *p; p++) {
    if (*p == '"' || *p == '\\') {
      g_string_append_c (out, '\\');
      g_string_append_c (out, *p);
    } else if (*p < 0x20) {
      g_string_append_printf (out, "\\u%04x", (unsigned int) *p);
    } else {
      g_string_append_c (out, *p);
    }
  }
  g_string_append_c (out, '"');
  return g_string_free (out, FALSE);
}

/*! \brief Append a conversation turn to a JSON array's interior.
 * \param history Mutable comma-separated turn objects, without brackets.
 * \param role Message role.
 * \param text Message content.
 * Side effects: extends history; the caller retains ownership.
 */
void
pcb_ai_chat_append_message (GString *history, const gchar *role,
                               const gchar *text)
{
  gchar *quoted_role = pcb_ai_chat_quote (role);
  gchar *quoted_text = pcb_ai_chat_quote (text);
  g_string_append_printf (history, "%s{\"role\":%s,\"content\":%s}",
                          history->len ? "," : "", quoted_role, quoted_text);
  g_free (quoted_role);
  g_free (quoted_text);
}
