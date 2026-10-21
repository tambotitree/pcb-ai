/* Private AI Assist state; GPL-2.0-or-later. */
#ifndef PCB_AI_CHAT_INTERNAL_H
#define PCB_AI_CHAT_INTERNAL_H
#include "gui.h"
#include <gio/gio.h>
#include "pcb_ai_chat.h"

typedef enum {
  PCB_AI_EDIT_NONE,
  PCB_AI_EDIT_PAD_WIDTH,
  PCB_AI_EDIT_VIA_DIAMETER
} PcbAiEditKind;

typedef struct PcbAiChat {
  GtkWindow *window;
  GtkWidget *panel, *settings, *provider, *endpoint, *models, *key, *env_file;
  GtkWidget *transcript, *input, *send, *cancel, *clear, *status;
  GtkWidget *send_image, *auto_image, *preview, *apply, *proposal;
  gchar *request_json, *request_board, *selection;
  gchar *patch_content, *patch_diff, *proposal_summary;
  GtkWidget *review;
  gboolean image_attempted;
  gdouble proposal_percent;
  PcbAiEditKind proposal_kind;
  GSubprocess *process;
  GKeyFile *config;
  GString *history;
  gchar *pending, *provider_id;
  PCBType *page_identity;
  gboolean closed, cancelled, listing_models;
} PcbAiChat;

GtkWidget *pcb_ai_chat_settings_new (PcbAiChat *chat);
void pcb_ai_chat_settings_save (PcbAiChat *chat);
void pcb_ai_chat_list_models (GtkButton *button, PcbAiChat *chat);
typedef struct {
  PcbAiEditKind kind;
  const gchar *name, *label;
  gdouble minimum, maximum;
  gboolean minimum_exclusive;
  const gchar *constraints;
  gboolean (*apply) (PcbAiChat *chat);
} PcbAiOperation;

const PcbAiOperation *pcb_ai_operation_by_name (const gchar *name);
gboolean pcb_ai_operation_accepts (const PcbAiOperation *operation, gdouble value);
gboolean pcb_ai_apply_pad_width (PcbAiChat *chat);
gchar *pcb_ai_viewport_png (PcbAiChat *chat);
gchar *pcb_ai_selection_manifest (void);
void pcb_ai_visual_controls (PcbAiChat *chat);
void pcb_ai_review_clicked (GtkButton *button, PcbAiChat *chat);
void pcb_ai_patch_clear (PcbAiChat *chat);
PCBType *pcb_ai_patch_parse (const gchar *text);
void pcb_ai_patch_free (PCBType *board);
gboolean pcb_ai_patch_apply (PcbAiChat *chat);
void pcb_ai_apply_clicked (GtkButton *button, PcbAiChat *chat);
gboolean pcb_ai_read_response (PcbAiChat *chat, const gchar *output, gchar **reply, gboolean *image);
void pcb_ai_chat_busy (PcbAiChat *chat, gboolean busy);
void pcb_ai_chat_append (PcbAiChat *chat, const gchar *speaker, const gchar *text);
gboolean pcb_ai_apply_via_diameter (PcbAiChat *chat);
#endif
