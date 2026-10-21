/* Native staging for reviewed design patches; GPL-2.0-or-later. */
#include <config.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include "pcb_ai_chat_internal.h"
#include "create.h"
#include "file.h"
#include "mymem.h"
#include "parse_l.h"
#include "set.h"
#include "undo.h"

/* Release an inactive board without RemovePCB's global undo-list side effect. */
void
pcb_ai_patch_free (PCBType *board)
{
  if (board != NULL)
    {
      FreePCBMemory (board);
      free (board);
    }
}

/* Discard only the unimplemented text proposal, never the live design. */
void
pcb_ai_patch_clear (PcbAiChat *chat)
{
  g_clear_pointer (&chat->proposal_summary, g_free);
  g_clear_pointer (&chat->patch_content, g_free);
  g_clear_pointer (&chat->patch_diff, g_free);
}

/* Parse a private temporary file, disabling configured shell file commands.
 * The live board and its font remain authoritative. No source file is written.
 */
PCBType *
pcb_ai_patch_parse (const gchar *text)
{
  gchar *path = NULL;
  gint fd = g_file_open_tmp ("pcb-ai-review-XXXXXX.pcb", &path, NULL);
  PCBType *live = PCB;
  PCBType *staged = NULL;
  char *command = Settings.FileCommand;
  char *search_path = Settings.FilePath;
  gboolean valid;
  guint i;
  if (fd < 0)
    return NULL;
  close (fd);
  valid = g_file_set_contents (path, text, -1, NULL);
  if (valid)
    {
      staged = CreateNewPCB ();
      PCB = staged;
      Settings.FileCommand = NULL;
      Settings.FilePath = NULL;
      valid = ParsePCB (staged, path) == 0;
      Settings.FileCommand = command;
      Settings.FilePath = search_path;
      PCB = live;
      valid = valid && staged->MaxWidth > 0 && staged->MaxHeight > 0 &&
        staged->Data->LayerN >= 2 && staged->Data->LayerN <= MAX_LAYER &&
        staged->Data->LayerN == live->Data->LayerN;
      if (valid)
        {
          /* Visibility is runtime state, absent from the serialized design.
           * A freshly parsed board otherwise hides every layer/object class.
           * Keep layer topology unchanged so the global LayerStack stays valid.
           */
          staged->ViaOn = live->ViaOn;
          staged->ElementOn = live->ElementOn;
          staged->RatOn = live->RatOn;
          staged->InvisibleObjectsOn = live->InvisibleObjectsOn;
          staged->PinOn = live->PinOn;
          staged->SilkActive = live->SilkActive;
          staged->RatDraw = live->RatDraw;
          for (i = 0; i < MAX_ALL_LAYER; i++)
            staged->Data->Layer[i].On = live->Data->Layer[i].On;
          /* Compact model context omits glyphs; preserve the real board font. */
          for (i = 0; i <= MAX_FONTPOSITION; i++)
            free (staged->Font.Symbol[i].Line);
          staged->Font = live->Font;
          for (i = 0; i <= MAX_FONTPOSITION; i++)
            {
              SymbolType *symbol = &staged->Font.Symbol[i];
              symbol->Line = symbol->LineMax > 0 ?
                g_memdup2 (live->Font.Symbol[i].Line, symbol->LineMax * sizeof (LineType)) : NULL;
            }
          free (staged->Filename);
          staged->Filename = live->Filename != NULL ? strdup (live->Filename) : NULL;
        }
      else
        {
          pcb_ai_patch_free (staged);
          staged = NULL;
        }
    }
  g_unlink (path);
  g_free (path);
  return staged;
}

/* Replace the live in-memory design only after review; one native Undo/Redo. */
gboolean
pcb_ai_patch_apply (PcbAiChat *chat)
{
  PCBType *staged = pcb_ai_patch_parse (chat->patch_content);
  PCBType *previous = PCB;
  if (staged == NULL)
    return FALSE;
  SetMode (ARROW_MODE);
  PCB = staged;
  AddBoardSwapToUndoList (previous);
  IncrementUndoSerialNumber ();
  hid_action ("PCBChanged");
  return TRUE;
}
