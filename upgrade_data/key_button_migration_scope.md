# Key, button and array-loop patterns

Completed as four separately compiled cycles (2026-10-20):

- Verified key-name rules in gtk_api_migrations.json replaced 23 references.
- The horizontal-button-box rule replaced all seven constructors.
- Four stock-button rules replaced nine constructors with explicit mnemonic text.
- tools/array_loop_conversion.py updated the four reviewed indices to gsize and
  G_N_ELEMENTS, preserving unrelated signed counters.

Command-window and keyref-window objects now compile strictly into arm64 objects.
No old button-box or stock-button constructor remains in source. GTK library
build next stops in gui-config.c: signed layer-count comparisons, deprecated
scrolled-window viewport insertion, and GtkArrow widgets. Main-window retains
separate stock-image APIs; no runtime icon work was undertaken.

Logs under scratch/build-gtk3: after-key-names.log, after-button-boxes.log,
after-text-buttons.log, after-array-loops.log, next-after-key-buttons.log.
Runtime UI behavior remains untested; the full GTK library/executable is not built.

Project-wide C/header audit, 2026-10-20. No source conversion applied in this audit.

1. **Old key names:** 23 code references to 19 names across command-window,
   library-window, gui-misc, gui-output-events and gui-utils. Each candidate has
   a corresponding GDK_KEY_ name in the installed GTK3 gdkkeysyms.h. The converter
   should use that explicit mapping, not rename all GDK_ tokens (event types and
   modifier masks keep their names). List: `scratch/legacy-key-candidates.json`.
2. **Button boxes:** seven horizontal constructors across command-window,
   keyref-window, pinout-window, netlist-window, log-window, DRC and preferences.
   Replace with gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL). No vertical case
   was found. Preserve existing button-box layout settings.
3. **Stock button constructors:** nine calls across seven files. The stock IDs
   are Close, Refresh, Add and Delete. Use explicit mnemonic text labels and
   retain click callbacks; no icon loading is needed for this conversion.
4. **Array-loop signedness:** three direct matches compare signed indices with
   sizeof array counts: command_ref_text, key_ref_text and layer_info_text.
   A related hotkey-table loop uses N_HOTKEY_ACTIONS. Review each index's other
   uses before choosing size_t/gsize and use element-based counts. Twenty-five
   broader sizeof/count matches include valid registration counts and pointer
   iteration; do not rewrite those merely because they matched the search.

Raw searches: `scratch/key-button-patterns.txt` and
`scratch/array-count-candidates.txt`. Convert and compile one pattern at a time,
starting with key names, then button boxes, stock buttons and the reviewed loops.
