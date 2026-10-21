/* Authoritative live selection manifest; GPL-2.0-or-later. */
#include <config.h>
#include "pcb_ai_chat_internal.h"

/* Describe stable pad IDs and widths; selecting an element selects its pads. */
gchar *
pcb_ai_selection_manifest (void)
{
  GString *text = g_string_new (NULL);
  GList *elements, *pads;
  int layer_index;
  if (PCB != NULL)
    for (elements = PCB->Data->Element; elements != NULL; elements = elements->next)
      {
        ElementType *element = elements->data;
        if (TEST_FLAG (SELECTEDFLAG, element))
          g_string_append_printf (text, "component id=%ld refdes=%s bounds=%ld,%ld..%ld,%ld locked=%d\n",
            (long) element->ID, NAMEONPCB_NAME (element) != NULL ? NAMEONPCB_NAME (element) : "?",
            (long) element->BoundingBox.X1, (long) element->BoundingBox.Y1,
            (long) element->BoundingBox.X2, (long) element->BoundingBox.Y2,
            TEST_FLAG (LOCKFLAG, element));
        for (pads = element->Pad; pads != NULL; pads = pads->next)
          {
            PadType *pad = pads->data;
            if (TEST_FLAG (SELECTEDFLAG, element) || TEST_FLAG (SELECTEDFLAG, pad))
              g_string_append_printf (text, "pad id=%ld refdes=%s pin=%s width_coord=%ld locked=%d\n",
                (long) pad->ID, NAMEONPCB_NAME (element) != NULL ? NAMEONPCB_NAME (element) : "?",
                pad->Number != NULL ? pad->Number : "?", (long) pad->Thickness,
                TEST_FLAG (LOCKFLAG, pad) || TEST_FLAG (LOCKFLAG, element));
          }
      }
  if (PCB != NULL)
    for (elements = PCB->Data->Via; elements != NULL; elements = elements->next)
      {
        PinType *via = elements->data;
        if (TEST_FLAG (SELECTEDFLAG, via))
          g_string_append_printf (text,
            "%s id=%ld x_coord=%ld y_coord=%ld diameter_coord=%ld drill_coord=%ld layers=%d..%d locked=%d\n",
            TEST_FLAG (HOLEFLAG, via) ? "hole" : "via", (long) via->ID,
            (long) via->X, (long) via->Y, (long) via->Thickness, (long) via->DrillingHole,
            via->BuriedFrom, via->BuriedTo, TEST_FLAG (LOCKFLAG, via));
      }
  if (PCB != NULL)
    for (layer_index = 0; layer_index < max_copper_layer; layer_index++)
      {
        LayerType *layer = &PCB->Data->Layer[layer_index];
        GList *lines;
        for (lines = layer->Line; lines != NULL; lines = lines->next)
          {
            LineType *line = lines->data;
            if (TEST_FLAG (SELECTEDFLAG, line))
              g_string_append_printf (text,
                "trace id=%ld layer=%d endpoints=%ld,%ld..%ld,%ld width_coord=%ld clearance_coord=%ld locked=%d\n",
                (long) line->ID, layer_index, (long) line->Point1.X, (long) line->Point1.Y,
                (long) line->Point2.X, (long) line->Point2.Y, (long) line->Thickness,
                (long) line->Clearance, TEST_FLAG (LOCKFLAG, line));
          }
      }
  return g_string_free (text, FALSE);
}
