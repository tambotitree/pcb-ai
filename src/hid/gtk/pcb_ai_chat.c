/* gEDA AI Assist asynchronous chat; GPL-2.0-or-later.
 *
 * panel
 * +-- settings grid (provider, endpoint, model, credentials)
 * +-- scrolled transcript
 * +-- message entry + Plan + Cancel + New chat
 * +-- status
 *
 * The helper process handles HTTP/JSON, not circuit edits. The GTK main loop
 * stays responsive. Each send captures the live page; history contains only
 * successful question/answer pairs, without accumulating board snapshots.
 */
#include <config.h>
#include "pcb_ai_chat_internal.h"
#include "file.h"

/*! \brief Append plain text to the transcript and scroll to the last turn.
 * \param chat Owner of the transcript.
 * \param speaker Visible speaker name.
 * \param text Plain text; never interpreted as markup or commands.
 */
void
pcb_ai_chat_append (PcbAiChat *chat, const gchar *speaker, const gchar *text)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (chat->transcript));
  GtkTextIter end;
  gchar *line;
  gtk_text_buffer_get_end_iter (buffer, &end);
  line = g_strdup_printf ("%s:\n%s\n\n", speaker, text);
  gtk_text_buffer_insert (buffer, &end, line, -1);
  gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (chat->transcript),
                                     gtk_text_buffer_get_insert (buffer));
  g_free (line);
}

/*! \brief Set request sensitivity without blocking board editing.
 * \param chat Chat controls.
 * \param busy TRUE while its helper is running.
 */
void
pcb_ai_chat_busy (PcbAiChat *chat, gboolean busy)
{
  gtk_widget_set_sensitive (chat->settings, !busy);
  gtk_widget_set_sensitive (chat->send, !busy);
  gtk_widget_set_sensitive (chat->input, !busy);
  gtk_widget_set_sensitive (chat->clear, !busy);
  gtk_widget_set_sensitive (chat->cancel, busy);
  gtk_widget_set_sensitive (chat->send_image, !busy);
  gtk_widget_set_sensitive (chat->auto_image, !busy);
  gtk_widget_set_sensitive (chat->apply, !busy && (chat->proposal_percent != 0 || chat->patch_content != NULL));
}

/*! \brief Release chat state after its panel and async callbacks are finished.
 * \param data Owned PcbAiChat, attached to the panel.
 */
static void
chat_free (gpointer data)
{
  PcbAiChat *chat = data;
  g_clear_object (&chat->process);
  g_key_file_unref (chat->config);
  g_string_free (chat->history, TRUE);
  g_free (chat->pending);
  g_free (chat->provider_id);
  g_free (chat->request_json);
  g_free (chat->request_board);
  pcb_ai_patch_clear (chat);
  g_free (chat->selection);
  g_free (chat);
}

/*! \brief Stop a helper before the parent window's plain C state is freed.
 * \param panel Destroyed panel (unused).
 * \param chat State kept alive by any outstanding async callback.
 */
static void
chat_destroy (GtkWidget *panel, PcbAiChat *chat)
{
  (void) panel;
  chat->closed = TRUE;
  chat->window = NULL;
  if (chat->process != NULL)
    g_subprocess_force_exit (chat->process);
}

/*! \brief Cancel only this chat's outstanding request.
 * \param button Cancel button (unused).
 * \param chat State; controls remain busy until the process has exited.
 */
static void
chat_cancel (GtkButton *button, PcbAiChat *chat)
{
  (void) button;
  if (chat->process != NULL) {
    chat->cancelled = TRUE;
    g_subprocess_force_exit (chat->process);
    gtk_label_set_text (GTK_LABEL (chat->status), _("Cancelling…"));
  }
}

/*! \brief Clear the conversation; provider settings are retained.
 * \param button New chat button (unused).
 * \param chat Idle chat state.
 */
static void
chat_clear (GtkButton *button, PcbAiChat *chat)
{
  GtkTextBuffer *buffer;
  (void) button;
  if (chat->process != NULL)
    return;
  g_string_truncate (chat->history, 0);
  pcb_ai_patch_clear (chat);
  chat->proposal_percent = 0;
  chat->proposal_kind = PCB_AI_EDIT_NONE;
  gtk_widget_set_sensitive (chat->apply, FALSE);
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (chat->transcript));
  gtk_text_buffer_set_text (buffer, "", 0);
  gtk_label_set_text (GTK_LABEL (chat->status), _("New chat uses the current board."));
}

/*! \brief Populate model IDs returned by the selected provider.
 * \param chat Chat owning an editable model dropdown.
 * \param text Newline-separated IDs; an existing typed model is preserved.
 */
static void
chat_models_received (PcbAiChat *chat, const gchar *text)
{
  GtkWidget *entry = gtk_bin_get_child (GTK_BIN (chat->models));
  gchar *previous = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  gchar **models = g_strsplit (text, "\n", -1);
  gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (chat->models));
  for (guint i = 0; models[i] != NULL; i++) {
    if (*models[i])
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (chat->models), models[i]);
  }
  gtk_entry_set_text (GTK_ENTRY (entry), previous);
  gtk_label_set_text (GTK_LABEL (chat->status),
    *text ? _("Choose a model from the dropdown, or type its ID.")
          : _("No models returned. Install a model or check provider access."));
  g_strfreev (models);
  g_free (previous);
}

static void chat_launch (PcbAiChat *chat, const gchar *request, gboolean list_models);
static void chat_field (GString *request, const gchar *name, const gchar *value);

/* Continue one frozen turn with a newly rendered image, at most once. */
static gboolean
chat_retry_image (PcbAiChat *chat)
{
  gchar *board;
  gchar *image;
  GString *request;
  if (chat->image_attempted || !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chat->auto_image)))
    return FALSE;
  board = pcb_board_snapshot ();
  if (board == NULL || g_strcmp0 (board, chat->request_board) != 0)
    {
      g_free (board);
      gtk_label_set_text (GTK_LABEL (chat->status), "Board changed while waiting. Send a new question for a fresh image.");
      return FALSE;
    }
  g_free (board);
  image = pcb_ai_viewport_png (chat);
  if (image == NULL)
    return FALSE;
  request = g_string_new_len (chat->request_json, strlen (chat->request_json) - 1);
  chat_field (request, "image_png", image);
  g_string_append (request, ",\"image_attempted\":true}");
  chat->image_attempted = TRUE;
  pcb_ai_chat_append (chat, "PCB-ai", "The model requested the current viewport. Sending one fresh PNG.");
  chat_launch (chat, request->str, FALSE);
  g_string_free (request, TRUE);
  g_free (image);
  return chat->process != NULL;
}

/*! \brief Complete a helper request on the main thread.
 * \param source Helper process.
 * \param result Async communication result.
 * \param data Referenced chat panel; its state survives dialog destruction.
 */
static void
chat_completed (GObject *source, GAsyncResult *result, gpointer data)
{
  GtkWidget *panel = data;
  PcbAiChat *chat = g_object_get_data (G_OBJECT (panel), "ai-chat");
  gchar *output = NULL, *diagnostic = NULL;
  GError *error = NULL;
  gboolean retry = FALSE;
  gboolean ok = g_subprocess_communicate_utf8_finish (G_SUBPROCESS (source), result,
                                                      &output, &diagnostic, &error);
  if (!chat->closed) {
    if (chat->cancelled) {
      gtk_label_set_text (GTK_LABEL (chat->status), _("Request cancelled."));
    } else if (ok && g_subprocess_get_successful (G_SUBPROCESS (source))) {
      if (chat->listing_models) {
        chat_models_received (chat, output);
      } else {
        gchar *reply = NULL;
        gboolean image = FALSE;
        if (!pcb_ai_read_response (chat, output, &reply, &image))
          gtk_label_set_text (GTK_LABEL (chat->status), "Invalid assistant response; no edit applied.");
        else if (image)
          {
            retry = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chat->auto_image)) && !chat->image_attempted;
            gtk_label_set_text (GTK_LABEL (chat->status), retry ? "Preparing requested image…" :
              "The model requested an image. Enable automatic images or use Send image with a vision model.");
          }
        else
          {
            pcb_ai_chat_append (chat, _("Assistant"), reply);
            pcb_ai_chat_append_message (chat->history, "user", chat->pending);
            pcb_ai_chat_append_message (chat->history, "assistant", reply);
            gtk_entry_set_text (GTK_ENTRY (chat->input), "");
            gtk_label_set_text (GTK_LABEL (chat->status), (chat->proposal_percent != 0 || chat->patch_content != NULL) ?
              "Plan ready. Review it, then press Act." : "Planning reply received. Continue the discussion.");
          }
        g_free (reply);
      }
    } else {
      const gchar *message = error != NULL ? error->message :
        (diagnostic != NULL && *diagnostic ? diagnostic : _("Model request failed."));
      gtk_label_set_text (GTK_LABEL (chat->status), message);
      pcb_ai_chat_append (chat, _("Error"), message);
    }
    pcb_ai_chat_busy (chat, FALSE);
  }
  g_clear_object (&chat->process);
  if (retry && !chat->closed)
    retry = chat_retry_image (chat);
  if (!retry)
    {
      g_clear_pointer (&chat->pending, g_free);
      g_clear_pointer (&chat->request_json, g_free);
    }
  g_clear_error (&error);
  g_free (output);
  g_free (diagnostic);
  g_object_unref (panel);
}

/*! \brief Add a string-valued field to the helper's JSON request.
 * \param request Mutable object, already containing its first field.
 * \param name Field name (an internal identifier).
 * \param value UTF-8 value; encoded without shell interpolation.
 */
static void
chat_field (GString *request, const gchar *name, const gchar *value)
{
  gchar *quoted = pcb_ai_chat_quote (value);
  g_string_append_printf (request, ",\"%s\":%s", name, quoted);
  g_free (quoted);
}

/*! \brief Start a model-list or chat request through a private stdin pipe.
 * \param chat Idle state with a valid current page for chat requests.
 * \param list_models TRUE fetches IDs without sending board data.
 * Side effects: settings saved, helper launched, controls marked busy.
 */
static void
chat_request (PcbAiChat *chat, gboolean list_models)
{
  GString *request;
  GtkWidget *model;
  const gchar *env;
  if (chat->process != NULL || chat->closed)
    return;
  model = gtk_bin_get_child (GTK_BIN (chat->models));
  if (!list_models && *gtk_entry_get_text (GTK_ENTRY (model)) == '\0')
    {
      gtk_label_set_text (GTK_LABEL (chat->status), "Choose a model using Load Models, or type a model ID first.");
      gtk_widget_grab_focus (model);
      return;
    }
  request = g_string_new (list_models ? "{\"operation\":\"models\""
                                               : "{\"operation\":\"assist\"");
  if (!list_models) {
    gchar *question = g_strdup (gtk_entry_get_text (GTK_ENTRY (chat->input)));
    gchar *board;
    gchar *report;
    PCBType *page = PCB;
    g_strstrip (question);
    board = pcb_board_snapshot ();
    if (*question == '\0' || board == NULL) {
      gtk_label_set_text (GTK_LABEL (chat->status), _("Open a board and enter a message."));
      g_free (board);
      g_free (question);
      g_string_free (request, TRUE);
      return;
    }
    if (chat->page_identity != page) {
      chat_clear (NULL, chat);
      chat->page_identity = page;
    }
    report = g_strdup ("Live PCB layout snapshot, including unsaved edits. Connectivity, layers and footprints are in the board text.");
    chat_field (request, "question", question);
    chat_field (request, "board", board);
    pcb_ai_patch_clear (chat);
  chat->proposal_percent = 0;
    chat->proposal_kind = PCB_AI_EDIT_NONE;
    gtk_widget_set_sensitive (chat->apply, FALSE);
    gtk_label_set_text (GTK_LABEL (chat->proposal), "Discussing this request. No executable plan yet.");
    g_free (chat->request_board);
    chat->request_board = g_strdup (board);
    g_free (chat->selection);
    chat->selection = pcb_ai_selection_manifest ();
    chat_field (request, "selection", chat->selection);
    chat->image_attempted = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chat->send_image));
    if (chat->image_attempted)
      {
        gchar *image = pcb_ai_viewport_png (chat);
        if (image == NULL)
          {
            gtk_label_set_text (GTK_LABEL (chat->status), "Cannot capture the board viewport.");
            g_free (question);
            g_free (board);
            g_free (report);
            g_string_free (request, TRUE);
            return;
          }
        chat_field (request, "image_png", image);
        g_string_append (request, ",\"image_attempted\":true");
        g_free (image);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chat->send_image), FALSE);
      }
    chat_field (request, "report", report);
    g_string_append_printf (request, ",\"history\":[%s]", chat->history->str);
    chat->pending = question;
    pcb_ai_chat_append (chat, _("You"), question);
    g_free (board);
    g_free (report);
  }
  model = gtk_bin_get_child (GTK_BIN (chat->models));
  env = gtk_entry_get_text (GTK_ENTRY (chat->env_file));
  chat_field (request, "provider", chat->provider_id);
  chat_field (request, "endpoint", gtk_entry_get_text (GTK_ENTRY (chat->endpoint)));
  chat_field (request, "model", gtk_entry_get_text (GTK_ENTRY (model)));
  chat_field (request, "env_file", env);
  chat_field (request, "data_dir", PCBLIBDIR);
  chat_field (request, "api_key", gtk_entry_get_text (GTK_ENTRY (chat->key)));
  g_string_append_c (request, '}');
  pcb_ai_chat_settings_save (chat);
  g_free (chat->request_json);
  chat->request_json = g_strdup (request->str);
  chat_launch (chat, request->str, list_models);
  g_string_free (request, TRUE);
}

/* Launch an asynchronous helper without placing keys or image data in argv. */
static void
chat_launch (PcbAiChat *chat, const gchar *request, gboolean list_models)
{
  gchar *helper;
  GError *error = NULL;
  helper = pcb_ai_helper_path ();
  chat->process = g_subprocess_new (G_SUBPROCESS_FLAGS_STDIN_PIPE |
    G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE,
    &error, PCB_AI_PYTHON, helper, NULL);
  g_free (helper);
  if (chat->process == NULL) {
    gtk_label_set_text (GTK_LABEL (chat->status), error->message);
    g_clear_error (&error);
    g_clear_pointer (&chat->pending, g_free);
    g_clear_pointer (&chat->request_json, g_free);
  } else {
    chat->listing_models = list_models;
    chat->cancelled = FALSE;
    pcb_ai_chat_busy (chat, TRUE);
    gtk_label_set_text (GTK_LABEL (chat->status),
      list_models ? _("Loading models…") : _("Waiting for the model…"));
    g_subprocess_communicate_utf8_async (chat->process, request, NULL,
                                        chat_completed, g_object_ref (chat->panel));
  }
}

/*! \brief Load model IDs for the selected provider; sends no board.
 * \param button Load Models button (unused).
 * \param chat Owner state.
 */
void
pcb_ai_chat_list_models (GtkButton *button, PcbAiChat *chat)
{
  (void) button;
  chat_request (chat, TRUE);
}

/*! \brief Send a message from the Send button.
 * \param button Signal source (unused).
 * \param chat Owner state.
 */
static void
chat_send_clicked (GtkButton *button, PcbAiChat *chat)
{
  (void) button;
  chat_request (chat, FALSE);
}

/*! \brief Send a message when Enter is pressed in the message entry.
 * \param entry Signal source (unused).
 * \param chat Owner state.
 */
static void
chat_send_activated (GtkEntry *entry, PcbAiChat *chat)
{
  (void) entry;
  chat_request (chat, FALSE);
}

/*! \brief Create the native chat panel; parent ownership controls its lifetime.
 * \param w_current Editor owning the current live board.
 * \return Chat panel with settings, transcript, input/actions and status.
 */
GtkWidget *
pcb_ai_chat_new (GtkWindow *w_current)
{
  PcbAiChat *chat = g_new0 (PcbAiChat, 1);
  GtkWidget *scrolled;
  GtkWidget *row;
  chat->window = w_current;
  chat->history = g_string_new (NULL);
  chat->panel = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  chat->status = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (chat->status), 0);
  gtk_label_set_line_wrap (GTK_LABEL (chat->status), TRUE);
  chat->settings = pcb_ai_chat_settings_new (chat);
  gtk_box_pack_start (GTK_BOX (chat->panel), chat->settings, FALSE, FALSE, 0);
  pcb_ai_visual_controls (chat);
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  chat->transcript = gtk_text_view_new ();
  gtk_text_view_set_editable (GTK_TEXT_VIEW (chat->transcript), FALSE);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (chat->transcript), GTK_WRAP_WORD_CHAR);
  gtk_container_add (GTK_CONTAINER (scrolled), chat->transcript);
  gtk_box_pack_start (GTK_BOX (chat->panel), scrolled, TRUE, TRUE, 0);
  row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  chat->input = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (chat->input), _("Ask about the current board…"));
  chat->send = gtk_button_new_with_label (_("Plan"));
  gtk_widget_set_tooltip_text (chat->send, "Discuss and prepare a plan; this never applies board edits");
  chat->cancel = gtk_button_new_with_label (_("Cancel"));
  chat->clear = gtk_button_new_with_label (_("New chat"));
  gtk_box_pack_start (GTK_BOX (row), chat->input, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (row), chat->send, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (row), chat->cancel, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (row), chat->clear, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (chat->panel), row, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (chat->panel), chat->status, FALSE, FALSE, 0);
  g_object_set_data_full (G_OBJECT (chat->panel), "ai-chat", chat, chat_free);
  g_signal_connect (chat->panel, "destroy", G_CALLBACK (chat_destroy), chat);
  g_signal_connect (chat->send, "clicked", G_CALLBACK (chat_send_clicked), chat);
  g_signal_connect (chat->input, "activate", G_CALLBACK (chat_send_activated), chat);
  g_signal_connect (chat->cancel, "clicked", G_CALLBACK (chat_cancel), chat);
  g_signal_connect (chat->clear, "clicked", G_CALLBACK (chat_clear), chat);
  pcb_ai_chat_busy (chat, FALSE);
  return chat->panel;
}
