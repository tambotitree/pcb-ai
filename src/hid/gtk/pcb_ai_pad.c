/* Selected-pad width operation; GPL-2.0-or-later. */
#include <config.h>
#include <math.h>
#include "pcb_ai_chat_internal.h"
#include "change.h"
#include "undo.h"

typedef struct {
  ElementType *element;
  PadType *pad;
  Coord width;
} PcbAiPadChange;

/* Preflight the entire selected pad set, then apply as one native undo step. */
gboolean
pcb_ai_apply_pad_width (PcbAiChat *chat)
{
  GArray *changes = g_array_new (FALSE, FALSE, sizeof (PcbAiPadChange));
  GList *elements, *pads;
  gboolean valid = TRUE;
  guint i;
  if (valid)
    for (elements = PCB->Data->Element; elements != NULL; elements = elements->next)
      {
        ElementType *element = elements->data;
        for (pads = element->Pad; pads != NULL; pads = pads->next)
          {
            PadType *pad = pads->data;
            if (TEST_FLAG (SELECTEDFLAG, element) || TEST_FLAG (SELECTEDFLAG, pad))
              {
                gdouble width = round ((gdouble) pad->Thickness * chat->proposal_percent / 100.0);
                PcbAiPadChange change = {element, pad, 0};
                if (TEST_FLAG (LOCKFLAG, element) || TEST_FLAG (LOCKFLAG, pad) ||
                    width < MIN_PADSIZE || width > MAX_PADSIZE ||
                    pad->Mask + width - pad->Thickness < 0)
                  valid = FALSE;
                else if ((Coord) width != pad->Thickness)
                  {
                    change.width = (Coord) width;
                    g_array_append_val (changes, change);
                  }
              }
          }
      }
  valid = valid && changes->len > 0;
  if (valid)
    {
      SaveUndoSerialNumber ();
      for (i = 0; i < changes->len; i++)
        {
          PcbAiPadChange *change = &g_array_index (changes, PcbAiPadChange, i);
          RestoreUndoSerialNumber ();
          if (!ChangeObjectSize (PAD_TYPE, change->element, change->pad, change->pad,
                                 change->width, true))
            {
              if (i > 0)
                {
                  IncrementUndoSerialNumber ();
                  Undo (true);
                }
              valid = FALSE;
              break;
            }
        }
    }
  g_array_unref (changes);
  return valid;
}
