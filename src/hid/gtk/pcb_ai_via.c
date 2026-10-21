/* Selected plated-via copper enlargement; GPL-2.0-or-later. */
#include <config.h>
#include <math.h>
#include "pcb_ai_chat_internal.h"
#include "change.h"
#include "undo.h"

typedef struct {
  PinType *via;
  Coord diameter;
} PcbAiViaChange;

/* Preflight the selected plated vias, preserving holes and layer spans.
 * Use the native operation to maintain polygons, spatial indices and mask.
 */
gboolean
pcb_ai_apply_via_diameter (PcbAiChat *chat)
{
  GArray *changes = g_array_new (FALSE, FALSE, sizeof (PcbAiViaChange));
  GList *vias;
  gboolean valid = chat->proposal_percent > 100 && chat->proposal_percent <= 200;
  guint i;
  for (vias = PCB->Data->Via; vias != NULL; vias = vias->next)
    {
      PinType *via = vias->data;
      if (TEST_FLAG (SELECTEDFLAG, via))
        {
          gdouble diameter = round ((gdouble) via->Thickness * chat->proposal_percent / 100.0);
          PcbAiViaChange change = {via, 0};
          if (TEST_FLAG (LOCKFLAG, via) || TEST_FLAG (HOLEFLAG, via) ||
              diameter < MIN_PINORVIASIZE || diameter > MAX_PINORVIASIZE ||
              diameter <= via->DrillingHole || diameter <= via->Thickness ||
              (via->Mask != 0 && via->Mask + diameter - via->Thickness < 0))
            valid = FALSE;
          else
            {
              change.diameter = (Coord) diameter;
              g_array_append_val (changes, change);
            }
        }
    }
  valid = valid && changes->len > 0;
  if (valid)
    {
      SaveUndoSerialNumber ();
      for (i = 0; i < changes->len; i++)
        {
          PcbAiViaChange *change = &g_array_index (changes, PcbAiViaChange, i);
          RestoreUndoSerialNumber ();
          if (!ChangeObjectSize (VIA_TYPE, change->via, change->via, change->via,
                                 change->diameter, true))
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
