# Combo-box migration scope

Completed text-combo conversion (2026-10-20): `tools/combo_conversion.py`
replaces nine deprecated calls across command-window and dialog-print, with
GtkComboBoxText casts on text operations. Active-index APIs remain GtkComboBox;
the changed-signal callback now explicitly accepts GtkComboBox. Routing's
model-backed picker and its header contracts are retained.

The command combo takes an owned, sunk reference for reparenting. History limits
use a nonnegative unsigned length for list comparisons; removals use checked
integer indices, and trimming can remove multiple excess entries after a limit
reduction. Existing duplicate-history promotion is retained.

Both targeted strict object builds were attempted. The obsolete combo APIs,
combo pointer-type errors and history-length signedness warning are gone. Neither
object is complete yet: window-class hints remain deprecated in both; command
window also has an old Escape constant, button-box/stock APIs and a separate
array-loop signedness warning. Log: `scratch/build-gtk3/after-combos.log`.
Runtime combo interaction/reparenting remains untested.

Project-wide source/header search: `scratch/combo-inventory.txt` (2026-10-20).

## Editable command history

`src/hid/gtk/gui-command-window.c` creates one editable text combo and keeps
its GtkEntry child. Commands can be typed or selected from history; the widget
moves between the command window and status line with an owned reference.
Replace entry_new_text with gtk_combo_box_text_new_with_entry, text operations
with GtkComboBoxText methods and their GTK_COMBO_BOX_TEXT casts. Keep GtkEntry
access through gtk_bin_get_child and keep ordinary active-index calls on
GtkComboBox. Review history-size signedness and widget reference ownership.

## Enumeration and unit pickers

`src/hid/gtk/gui-dialog-print.c` creates two non-editable text combos, used by
HID enumeration/mixed attributes and units. Use gtk_combo_box_text_new and
gtk_combo_box_text_append_text, retaining GtkComboBox active-index methods and
the existing typed integer result storage.

## Model-backed routing picker

`src/hid/gtk/ghid-route-style-selector.c` uses gtk_combo_box_new_with_model and
GtkTreeIter selection. These are supported GTK3 APIs; it is not a text-combo
conversion candidate.

## Declarations and callers

`src/hid/gtk/gui.h` stores the command combo as GtkWidget and its child as
GtkEntry. gui-top-window packs that widget in containers. Those declarations
and callers are valid; do not replace general container casts with text-combo
casts. No matching combo API use was found in another backend.

The existing GTK3 widget families cover these uses; a new generic combo wrapper
is not required. Implement the confirmed text-helper patterns in Python and
compile the command-window and dialog-print objects separately.
