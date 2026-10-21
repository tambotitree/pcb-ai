/* gEDA AI Assist provider controls; GPL-2.0-or-later. */
#include <config.h>
#include <glib/gstdio.h>
#include "pcb_ai_chat_internal.h"

/*! \brief Return the user settings filename; caller frees it. */
static gchar *
settings_path (void)
{
  return g_build_filename (g_get_user_config_dir (), "pcb-ai", "ai-assist.ini", NULL);
}

/*! \brief Remember the current provider's endpoint and model in memory.
 * \param chat Chat owning the settings; no key is retained in the keyfile.
 */
static void
remember_provider (PcbAiChat *chat)
{
  GtkWidget *entry;
  if (chat->provider_id == NULL)
    return;
  entry = gtk_bin_get_child (GTK_BIN (chat->models));
  g_key_file_set_string (chat->config, chat->provider_id, "endpoint",
                         gtk_entry_get_text (GTK_ENTRY (chat->endpoint)));
  g_key_file_set_string (chat->config, chat->provider_id, "model",
                         gtk_entry_get_text (GTK_ENTRY (entry)));
}

/*! \brief Save provider choices and credential-file path, never API keys.
 * \param chat Chat owning the controls. Reports write failures in the UI.
 */
void
pcb_ai_chat_settings_save (PcbAiChat *chat)
{
  const gchar *env;
  gchar *path;
  gchar *directory;
  GError *error = NULL;
  remember_provider (chat);
  g_key_file_set_string (chat->config, "settings", "provider", chat->provider_id);
  env = gtk_entry_get_text (GTK_ENTRY (chat->env_file));
  g_key_file_set_string (chat->config, "settings", "env-file", env ? env : "");
  path = settings_path ();
  directory = g_path_get_dirname (path);
  if (g_mkdir_with_parents (directory, 0700) != 0 ||
      !g_key_file_save_to_file (chat->config, path, &error))
    gtk_label_set_text (GTK_LABEL (chat->status), _("Could not save AI settings."));
  g_clear_error (&error);
  g_free (directory);
  g_free (path);
}

/*! \brief Select a provider, restoring its endpoint/model and clearing typed keys.
 * \param combo Provider dropdown.
 * \param chat Owner; conversation text remains available across providers.
 */
static void
provider_changed (GtkComboBox *combo, PcbAiChat *chat)
{
  static const gchar *const endpoints[] = {
    "http://127.0.0.1:11434/api/chat", "https://api.openai.com/v1/chat/completions",
    "https://api.deepseek.com/chat/completions",
    "https://openrouter.ai/api/v1/chat/completions", "http://127.0.0.1:8080/v1/chat/completions"
  };
  gint index;
  gchar *endpoint;
  gchar *model;
  GtkWidget *entry;
  remember_provider (chat);
  g_free (chat->provider_id);
  chat->provider_id = g_strdup (gtk_combo_box_get_active_id (combo));
  index = gtk_combo_box_get_active (combo);
  endpoint = g_key_file_get_string (chat->config, chat->provider_id, "endpoint", NULL);
  model = g_key_file_get_string (chat->config, chat->provider_id, "model", NULL);
  gtk_entry_set_text (GTK_ENTRY (chat->endpoint), endpoint ? endpoint : endpoints[index]);
  gtk_combo_box_text_remove_all (GTK_COMBO_BOX_TEXT (chat->models));
  entry = gtk_bin_get_child (GTK_BIN (chat->models));
  gtk_entry_set_text (GTK_ENTRY (entry), model ? model : "");
  gtk_entry_set_text (GTK_ENTRY (chat->key), "");
  gtk_label_set_text (GTK_LABEL (chat->status),
    _("Load Models to choose a model. Select a .env or enter a key if needed."));
  g_free (endpoint);
  g_free (model);
}

/*! \brief Add a labelled settings control to the grid.
 * \param grid Parent grid; label is on the left, control on the right.
 * \param row Grid row.
 * \param title Visible label.
 * \param control Widget receiving the label's mnemonic.
 */
static void
settings_row (GtkWidget *grid, gint row, const gchar *title, GtkWidget *control)
{
  GtkWidget *label = gtk_label_new (title);
  gtk_label_set_xalign (GTK_LABEL (label), 0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_set_hexpand (control, TRUE);
  gtk_grid_attach (GTK_GRID (grid), control, 1, row, 1, 1);
}

/*! \brief Build provider controls and load saved non-secret preferences.
 * \param chat State initialized with a status label.
 * \return Grid owned by its eventual chat panel parent.
 * Hierarchy: grid -> provider, endpoint, model + Load Models, .env path, key.
 */
GtkWidget *
pcb_ai_chat_settings_new (PcbAiChat *chat)
{
  gchar *path;
  GtkWidget *grid;
  GtkWidget *load;
  gchar *env;
  gchar *provider;
  chat->config = g_key_file_new ();
  path = settings_path ();
  g_key_file_load_from_file (chat->config, path, G_KEY_FILE_NONE, NULL);
  g_free (path);
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 8);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);
  chat->provider = gtk_combo_box_text_new ();
  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (chat->provider), "ollama", "Ollama");
  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (chat->provider), "openai", "OpenAI");
  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (chat->provider), "deepseek", "DeepSeek");
  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (chat->provider), "openrouter", "OpenRouter");
  gtk_combo_box_text_append (GTK_COMBO_BOX_TEXT (chat->provider), "compatible", _("Other compatible"));
  settings_row (grid, 0, _("Provider"), chat->provider);
  chat->endpoint = gtk_entry_new ();
  settings_row (grid, 1, _("Chat endpoint"), chat->endpoint);
  chat->models = gtk_combo_box_text_new_with_entry ();
  settings_row (grid, 2, _("Model"), chat->models);
  load = gtk_button_new_with_label (_("Load Models"));
  gtk_grid_attach (GTK_GRID (grid), load, 2, 2, 1, 1);
  g_signal_connect (load, "clicked", G_CALLBACK (pcb_ai_chat_list_models), chat);
  chat->env_file = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (chat->env_file),
                                  _("Optional .env path (also searches launch directory, data directory and home)"));
  env = g_key_file_get_string (chat->config, "settings", "env-file", NULL);
  if (env != NULL)
    gtk_entry_set_text (GTK_ENTRY (chat->env_file), env);
  g_free (env);
  settings_row (grid, 3, _("Credentials .env"), chat->env_file);
  chat->key = gtk_entry_new ();
  gtk_entry_set_visibility (GTK_ENTRY (chat->key), FALSE);
  gtk_entry_set_placeholder_text (GTK_ENTRY (chat->key),
    _("Optional: environment or .env key is used when blank"));
  settings_row (grid, 4, _("API key"), chat->key);
  provider = g_key_file_get_string (chat->config, "settings", "provider", NULL);
  g_signal_connect (chat->provider, "changed", G_CALLBACK (provider_changed), chat);
  if (provider == NULL ||
      !gtk_combo_box_set_active_id (GTK_COMBO_BOX (chat->provider), provider))
    gtk_combo_box_set_active (GTK_COMBO_BOX (chat->provider), 0);
  g_free (provider);
  return grid;
}
