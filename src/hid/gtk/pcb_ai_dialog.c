/* PCB AI Assist, adapted from gEDA-ai; GPL-2.0-or-later. */
#include <config.h>
#include "gui.h"
#include "pcb_ai_chat.h"

/* Resolve installed transport first, then the source-tree development copy. */
gchar *
pcb_ai_helper_path (void)
{
  gchar *installed = g_build_filename (PCBLIBDIR, "ai", "chat_backend.py", NULL);
  if (g_file_test (installed, G_FILE_TEST_IS_REGULAR))
    return installed;
  g_free (installed);
  return g_strdup (PCB_AI_SOURCE_HELPER);
}

/* Clear the parent's borrowed dialog pointer before child destruction finishes. */
static void
assist_destroyed (GtkWidget *dialog G_GNUC_UNUSED, GtkWindow *parent)
{
  g_object_set_data (G_OBJECT (parent), "pcb-ai-dialog", NULL);
}

/* Close on a dialog response; panel destruction cancels any pending helper. */
static void
assist_response (GtkDialog *dialog, gint response G_GNUC_UNUSED,
                 gpointer data G_GNUC_UNUSED)
{
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

/* Present one chat dialog per editor; every Send captures current unsaved data.
 * Hierarchy: dialog -> content box -> explanation, copied chat panel.
 */
void
pcb_ai_assist_dialog (GtkWindow *parent)
{
  GtkWidget *dialog = g_object_get_data (G_OBJECT (parent), "pcb-ai-dialog");
  GtkWidget *content;
  GtkWidget *label;
  if (dialog != NULL)
    {
      gtk_window_present (GTK_WINDOW (dialog));
      return;
    }
  dialog = gtk_dialog_new_with_buttons ("PCB-ai — AI Assist", parent,
    GTK_DIALOG_DESTROY_WITH_PARENT, "Close", GTK_RESPONSE_CLOSE, NULL);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 900, 720);
  content = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (content), 8);
  label = gtk_label_new ("Each message sends the current PCB, including unsaved edits, "
                        "to the selected provider. Plan discusses changes; Act opens review; Implement applies the reviewed plan. Edits can be undone.");
  gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
  gtk_box_pack_start (GTK_BOX (content), label, FALSE, FALSE, 6);
  gtk_box_pack_start (GTK_BOX (content), pcb_ai_chat_new (parent), TRUE, TRUE, 0);
  g_object_set_data (G_OBJECT (parent), "pcb-ai-dialog", dialog);
  g_signal_connect (dialog, "response", G_CALLBACK (assist_response), NULL);
  g_signal_connect (dialog, "destroy", G_CALLBACK (assist_destroyed), parent);
  gtk_widget_show_all (dialog);
}
