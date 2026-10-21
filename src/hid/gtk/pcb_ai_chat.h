/* gEDA AI Assist chat; GPL-2.0-or-later. */
#ifndef PCB_AI_CHAT_H
#define PCB_AI_CHAT_H

#include <gtk/gtk.h>


GtkWidget *pcb_ai_chat_new (GtkWindow *w_current);
gchar *pcb_ai_chat_quote (const gchar *text);
void pcb_ai_chat_append_message (GString *history, const gchar *role,
                                    const gchar *text);

gchar *pcb_ai_helper_path (void);
void pcb_ai_assist_dialog (GtkWindow *parent);
#endif
