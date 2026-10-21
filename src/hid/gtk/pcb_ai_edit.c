/* Shared operation registry and guarded Apply boundary; GPL-2.0-or-later. */
#include <config.h>
#include <math.h>
#include "pcb_ai_chat_internal.h"
#include "file.h"
#include "set.h"

static const PcbAiOperation operations[] = {
  {PCB_AI_EDIT_PAD_WIDTH, "scale_selected_pad_width", "selected pad WIDTH", 10, 200, FALSE,
   "endpoints fixed; mask follows width", pcb_ai_apply_pad_width},
  {PCB_AI_EDIT_VIA_DIAMETER, "scale_selected_via_diameter", "selected via OUTER COPPER DIAMETER", 100, 200, TRUE,
   "drill, position and layer span fixed; mask follows diameter", pcb_ai_apply_via_diameter}
};

/* Validate numeric proposal parameters against the registered contract. */
gboolean
pcb_ai_operation_accepts (const PcbAiOperation *operation, gdouble value)
{
  return operation != NULL && isfinite (value) && value <= operation->maximum &&
    (operation->minimum_exclusive ? value > operation->minimum : value >= operation->minimum);
}

/* Resolve only explicitly registered operations; model text cannot name C calls. */
const PcbAiOperation *
pcb_ai_operation_by_name (const gchar *name)
{
  gsize i;
  for (i = 0; i < G_N_ELEMENTS (operations); i++)
    if (g_strcmp0 (name, operations[i].name) == 0)
      return &operations[i];
  return NULL;
}

/* Resolve the typed local proposal state for the shared Apply dispatcher. */
static const PcbAiOperation *
operation_by_kind (PcbAiEditKind kind)
{
  gsize i;
  for (i = 0; i < G_N_ELEMENTS (operations); i++)
    if (kind == operations[i].kind)
      return &operations[i];
  return NULL;
}

/* Guard the frozen board and selection once, then dispatch a typed operation.
 * Each operation preflights its entire target set and provides one undo step.
 */
void
pcb_ai_apply_clicked (GtkButton *button G_GNUC_UNUSED, PcbAiChat *chat)
{
  gchar *board = pcb_board_snapshot ();
  gchar *selection = pcb_ai_selection_manifest ();
  const PcbAiOperation *operation = operation_by_kind (chat->proposal_kind);
  gboolean valid = chat->process == NULL && (chat->patch_content != NULL ||
      pcb_ai_operation_accepts (operation, chat->proposal_percent)) &&
    PCB == chat->page_identity && board != NULL &&
    g_strcmp0 (board, chat->request_board) == 0 &&
    (chat->patch_content != NULL || *selection != '\0') &&
    g_strcmp0 (selection, chat->selection) == 0;
  if (!valid)
    gtk_label_set_text (GTK_LABEL (chat->status), "Cannot apply: board or selection changed, or proposal is invalid. Ask again.");
  else
    {
      valid = chat->patch_content != NULL ? pcb_ai_patch_apply (chat) : operation->apply (chat);
      if (valid)
        SetChangedFlag (true);
      ghid_invalidate_all ();
      g_free (pcb_ai_viewport_png (chat));
      gtk_label_set_text (GTK_LABEL (chat->status), valid ?
        "Edit applied; board and image updated. Undo restores it. Run DRC before manufacturing." :
        "Cannot apply: no matching objects, locked objects, invalid dimensions or resize rejected. No partial edit retained.");
      pcb_ai_chat_append (chat, "PCB-ai", valid ?
        "Applied the reviewed operation. Board and preview updated; next message uses the new geometry." :
        "The proposal could not be applied.");
    }
  pcb_ai_patch_clear (chat);
  chat->proposal_percent = 0;
  chat->proposal_kind = PCB_AI_EDIT_NONE;
  gtk_widget_set_sensitive (chat->apply, FALSE);
  g_free (selection);
  g_free (board);
}
