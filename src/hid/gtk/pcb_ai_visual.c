/* Live PCB viewport images and constrained reply decoding; GPL-2.0-or-later. */
#include <config.h>
#include "pcb_ai_chat_internal.h"

/* Bound PNG serialization to eight MiB; Cairo owns the source surface. */
static cairo_status_t
png_bytes (void *closure, const unsigned char *data, unsigned int length)
{
  GByteArray *bytes = closure;
  if (bytes->len + (guint64) length > 8 * 1024 * 1024)
    return CAIRO_STATUS_WRITE_ERROR;
  g_byte_array_append (bytes, data, length);
  return CAIRO_STATUS_SUCCESS;
}

/* Render from the current board, not an old screenshot; caller frees base64. */
gchar *
pcb_ai_viewport_png (PcbAiChat *chat)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  GByteArray *bytes;
  GdkPixbuf *pixbuf;
  gchar *encoded = NULL;
  gdouble scale;
  gint width, height;
  if (gport == NULL || gport->pixmap == NULL || gport->width <= 0 || gport->height <= 0)
    return NULL;
  ghid_invalidate_all ();
  scale = MIN (1.0, 1280.0 / MAX (gport->width, gport->height));
  width = MAX (1, (gint) (gport->width * scale));
  height = MAX (1, (gint) (gport->height * scale));
  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);
  cr = cairo_create (surface);
  cairo_scale (cr, scale, scale);
  cairo_set_source_surface (cr, gport->pixmap, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  bytes = g_byte_array_new ();
  if (cairo_surface_write_to_png_stream (surface, png_bytes, bytes) == CAIRO_STATUS_SUCCESS)
    encoded = g_base64_encode (bytes->data, bytes->len);
  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, width, height);
  if (pixbuf != NULL)
    {
      GdkPixbuf *small = gdk_pixbuf_scale_simple (pixbuf, MAX (1, width / 3),
                                                 MAX (1, height / 3), GDK_INTERP_BILINEAR);
      gtk_image_set_from_pixbuf (GTK_IMAGE (chat->preview), small);
      g_object_unref (small);
      g_object_unref (pixbuf);
    }
  g_byte_array_unref (bytes);
  cairo_surface_destroy (surface);
  return encoded;
}

/* Add explicit image consent/override and a visible, reviewable edit control.
 * panel -> options row, expandable image, proposal row.
 */
void
pcb_ai_visual_controls (PcbAiChat *chat)
{
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  GtkWidget *expander = gtk_expander_new ("Current viewport image");
  GtkWidget *proposal_row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  chat->send_image = gtk_check_button_new_with_label ("Send image with next message");
  chat->auto_image = gtk_check_button_new_with_label ("Allow automatic image request (vision model)");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chat->auto_image), TRUE);
  gtk_box_pack_start (GTK_BOX (row), chat->send_image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (row), chat->auto_image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (chat->panel), row, FALSE, FALSE, 0);
  chat->preview = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (expander), chat->preview);
  gtk_box_pack_start (GTK_BOX (chat->panel), expander, FALSE, FALSE, 0);
  chat->proposal = gtk_label_new ("Plan first. Discuss the change; Act becomes available for a concrete proposal.");
  gtk_label_set_line_wrap (GTK_LABEL (chat->proposal), TRUE);
  chat->apply = gtk_button_new_with_label ("Act");
  gtk_widget_set_sensitive (chat->apply, FALSE);
  gtk_widget_set_tooltip_text (chat->apply, "Apply only the reviewed plan to the unchanged board and selection");
  gtk_box_pack_start (GTK_BOX (proposal_row), chat->proposal, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (proposal_row), chat->apply, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (chat->panel), proposal_row, FALSE, FALSE, 0);
  g_signal_connect (chat->apply, "clicked", G_CALLBACK (pcb_ai_review_clicked), chat);
}

/* Parse only the helper's INI envelope; never execute model text as actions. */
gboolean
pcb_ai_read_response (PcbAiChat *chat, const gchar *output, gchar **reply, gboolean *image)
{
  GKeyFile *fields = g_key_file_new ();
  gchar *kind, *encoded, *operation;
  guchar *decoded;
  gsize length;
  gdouble percent;
  const PcbAiOperation *edit;
  gboolean valid;
  if (!g_key_file_load_from_data (fields, output, -1, G_KEY_FILE_NONE, NULL))
    {
      g_key_file_unref (fields);
      return FALSE;
    }
  kind = g_key_file_get_string (fields, "response", "kind", NULL);
  encoded = g_key_file_get_string (fields, "response", "text", NULL);
  operation = g_key_file_get_string (fields, "response", "operation", NULL);
  edit = pcb_ai_operation_by_name (operation);
  chat->proposal_kind = edit != NULL ? edit->kind : PCB_AI_EDIT_NONE;
  percent = g_key_file_get_double (fields, "response", "percent", NULL);
  valid = kind != NULL && encoded != NULL &&
    (strcmp (kind, "image") == 0 || strcmp (kind, "reply") == 0);
  *image = valid && strcmp (kind, "image") == 0;
  decoded = g_base64_decode (encoded != NULL ? encoded : "", &length);
  valid = valid && g_utf8_validate ((const gchar *) decoded, length, NULL) &&
    memchr (decoded, 0, length) == NULL;
  if (valid)
    *reply = g_strndup ((const gchar *) decoded, length);
  pcb_ai_patch_clear (chat);
  if (valid)
    chat->proposal_summary = g_strdup (*reply);
  if (valid && !*image && g_key_file_get_integer (fields, "response", "patch_count", NULL) == 1)
    {
      gchar *path64 = g_key_file_get_string (fields, "response", "patch_0_path", NULL);
      gchar *content64 = g_key_file_get_string (fields, "response", "patch_0_content", NULL);
      gchar *diff64 = g_key_file_get_string (fields, "response", "patch_0_diff", NULL);
      gsize path_length, content_length, diff_length;
      guchar *path = g_base64_decode (path64 != NULL ? path64 : "", &path_length);
      guchar *content = g_base64_decode (content64 != NULL ? content64 : "", &content_length);
      guchar *diff = g_base64_decode (diff64 != NULL ? diff64 : "", &diff_length);
      if (path_length == strlen ("current.pcb") && memcmp (path, "current.pcb", path_length) == 0 &&
          content_length > 0 && content_length <= 4 * 1024 * 1024 && diff_length <= 4 * 1024 * 1024 &&
          memchr (content, 0, content_length) == NULL && memchr (diff, 0, diff_length) == NULL &&
          g_utf8_validate ((const gchar *) content, content_length, NULL) &&
          g_utf8_validate ((const gchar *) diff, diff_length, NULL) && edit == NULL)
        {
          chat->patch_content = g_strndup ((const gchar *) content, content_length);
          chat->patch_diff = g_strndup ((const gchar *) diff, diff_length);
          gtk_label_set_text (GTK_LABEL (chat->proposal), "PCB patch ready. Act opens the diff and native parser validation; Implement applies it.");
        }
      else
        valid = FALSE;
      g_free (path64); g_free (content64); g_free (diff64);
      g_free (path); g_free (content); g_free (diff);
    }
  chat->proposal_percent = valid && !*image && pcb_ai_operation_accepts (edit, percent) ? percent : 0;
  if (chat->proposal_percent != 0)
    {
      gchar *description = g_strdup_printf (
        "Plan: %s to %.2f%%; %s. Press Act to review; Undo available; run DRC.", edit->label, percent, edit->constraints);
      gtk_label_set_text (GTK_LABEL (chat->proposal), description);
      g_free (description);
    }
  gtk_widget_set_sensitive (chat->apply, (chat->proposal_percent != 0 || chat->patch_content != NULL));
  g_free (decoded);
  g_free (encoded);
  g_free (kind);
  g_free (operation);
  g_key_file_unref (fields);
  return valid;
}
