/* Explicit review/Implement boundary; GPL-2.0-or-later. */
#include <config.h>
#include "pcb_ai_chat_internal.h"
#include "file.h"

/* Add read-only, selectable monospaced text to the review notebook. */
static void
review_text (GtkNotebook *book, const gchar *title, const gchar *text)
{
  GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *view = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
  gtk_text_view_set_monospace (GTK_TEXT_VIEW (view), TRUE);
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)), text, -1);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_notebook_append_page (book, scroll, gtk_label_new (title));
}

/* Render staged geometry offscreen at the same viewport, then restore PCB. */
static void
review_image (GtkNotebook *book, const gchar *title, PCBType *board)
{
  PCBType *live = PCB;
  cairo_surface_t *surface;
  GdkPixbuf *pixbuf;
  GtkWidget *image;
  GtkWidget *scroll;
  Coord cx, cy;
  double zoom;
  gint width = 700, height = 400;
  if (gport == NULL || gport->pixmap == NULL)
    return;
  cx = gport->view.x0 + gport->view.width / 2;
  cy = gport->view.y0 + gport->view.height / 2;
  zoom = MAX (gport->view.width / width, gport->view.height / height);
  PCB = board;
  surface = ghid_render_pixmap (cx, cy, zoom, width, height, 24);
  PCB = live;
  if (surface == NULL)
    return;
  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, width, height);
  image = gtk_image_new_from_pixbuf (pixbuf);
  scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scroll), image);
  gtk_notebook_append_page (book, scroll, gtk_label_new (title));
  g_clear_object (&pixbuf);
  cairo_surface_destroy (surface);
}

/* Only this explicit response crosses the live-edit boundary. */
static void
review_response (GtkDialog *dialog, gint response, PcbAiChat *chat)
{
  if (!chat->closed && response == GTK_RESPONSE_ACCEPT)
    pcb_ai_apply_clicked (NULL, chat);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

/* Keep chat state alive while its separate review window exists. */
static void
review_destroyed (GtkWidget *dialog G_GNUC_UNUSED, PcbAiChat *chat)
{
  chat->review = NULL;
  if (!chat->closed)
    gtk_widget_set_sensitive (chat->panel, TRUE);
  g_object_unref (chat->panel);
}

/* Act stages and displays; cancellation leaves the live board untouched. */
void
pcb_ai_review_clicked (GtkButton *button G_GNUC_UNUSED, PcbAiChat *chat)
{
  GtkWidget *dialog, *content, *book, *label;
  gchar *current = pcb_board_snapshot ();
  gchar *selection = pcb_ai_selection_manifest ();
  gchar *summary;
  PCBType *staged = NULL;
  gboolean valid = !chat->closed && chat->process == NULL && PCB == chat->page_identity &&
    current != NULL && g_strcmp0 (current, chat->request_board) == 0 &&
    g_strcmp0 (selection, chat->selection) == 0 &&
    (chat->patch_content != NULL || chat->proposal_percent != 0);
  g_free (current);
  g_free (selection);
  if (!valid)
    {
      gtk_label_set_text (GTK_LABEL (chat->status), "Board or selection changed. Plan again before reviewing.");
      return;
    }
  if (chat->review != NULL)
    {
      gtk_window_present (GTK_WINDOW (chat->review));
      return;
    }
  if (chat->patch_content != NULL)
    staged = pcb_ai_patch_parse (chat->patch_content);
  valid = chat->patch_content == NULL || staged != NULL;
  dialog = gtk_dialog_new_with_buttons ("Review proposed design changes",
    GTK_WINDOW (gtk_widget_get_toplevel (chat->panel)), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    "Back to planning", GTK_RESPONSE_CANCEL, "Implement", GTK_RESPONSE_ACCEPT, NULL);
  chat->review = dialog;
  gtk_window_set_default_size (GTK_WINDOW (dialog), 850, 650);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, valid);
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (content), 10);
  summary = g_strdup_printf ("%s\nAffected design: %s\n%s\nImplement changes the live board; save normally. One Undo restores it.",
    (chat->proposal_summary != NULL ? chat->proposal_summary : gtk_label_get_text (GTK_LABEL (chat->proposal))), PCB->Filename != NULL ? PCB->Filename : "Untitled PCB",
    chat->patch_content != NULL ? (valid ? "Exact patch and native PCB parser: passed. DRC has not been run." :
      "Native PCB parser rejected this patch. Implement is disabled.") :
      "Native operation: bounds, selection and geometry are rechecked at Implement.");
  label = gtk_label_new (summary);
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_label_set_max_width_chars (GTK_LABEL (label), 100);
  gtk_label_set_lines (GTK_LABEL (label), 12);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_box_pack_start (GTK_BOX (content), label, FALSE, FALSE, 8);
  g_free (summary);
  book = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (content), book, TRUE, TRUE, 0);
  review_text (GTK_NOTEBOOK (book), chat->patch_diff != NULL ? "Exact diff" : "Selected objects",
               chat->patch_diff != NULL ? chat->patch_diff : chat->selection);
  review_image (GTK_NOTEBOOK (book), "Before", PCB);
  if (staged != NULL)
    review_image (GTK_NOTEBOOK (book), "After", staged);
  pcb_ai_patch_free (staged);
  g_object_ref (chat->panel);
  gtk_widget_set_sensitive (chat->panel, FALSE);
  g_signal_connect (dialog, "response", G_CALLBACK (review_response), chat);
  g_signal_connect (dialog, "destroy", G_CALLBACK (review_destroyed), chat);
  gtk_widget_show_all (dialog);
}
