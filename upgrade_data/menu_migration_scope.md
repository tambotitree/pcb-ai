# Menu migration scope

Generated from a project-wide search of C sources and headers on 2026-10-20.
This is a dependency work set, not a list of confirmed defects.

```sh
rg -l -i 'gtk_[a-z_]*(menu|action)|Gtk(Menu|Action|RadioAction|ToggleAction)|tearoff' src --glob '*.[ch]'
```

## GTK sources and headers

- `src/hid/gtk/ghid-coord-entry.c`
- `src/hid/gtk/ghid-layer-selector.c`
- `src/hid/gtk/ghid-layer-selector.h`
- `src/hid/gtk/ghid-main-menu.c`
- `src/hid/gtk/ghid-main-menu.h`
- `src/hid/gtk/ghid-route-style-selector.c`
- `src/hid/gtk/ghid-route-style-selector.h`
- `src/hid/gtk/gtkhid-main.c`
- `src/hid/gtk/gui-dialog.c`
- `src/hid/gtk/gui-top-window.c`
- `src/hid/gtk/gui.h`

## Menu definitions and other backends

- `src/gpcb-menu.res.in`: GTK menu definitions; preserve commands and shortcuts.
- `src/hid/lesstif/menu.c`: separate backend; inspect shared resource implications.
- `src/pcb-menu.res.in`: LessTif menu definitions.

## Conversion order

Three separate GTK cleanup cycles were applied through reviewed additions to
`gtk_api_migrations.json`, with a targeted gtkhid-main object build after each:

- Both process_all_updates calls now queue the drawing widget for GTK painting
  (the separate GL implementation remains disabled and uncompiled).
- The single global beep uses gdk_display_beep on the drawing widget's display.
- The sole GtkAlignment wrapper uses a vertical GtkBox and eight-pixel margins.

Project-wide C/header searches find no remaining instances of those three APIs.
Their diagnostics are gone, but gtkhid-main still does not compile: incomplete
HID data initializers, the separate benchmark process_updates call, old key
constants, and gtk_menu_popup remain. Logs: `build-gtk3/after-redraw-queue.log`,
`after-display-beep.log` and `after-alignment.log`. No runtime validation or
linked GUI is claimed.

Pointer-device cycle: `tools/pointer_conversion.py` replaces all eleven confirmed
calls across gtkhid-main, gui-misc, gui-output-events and gui-utils. The typed
`pcb-pointer.h` helpers obtain a borrowed pointer device from the display's
default seat and call gdk_window_get_device_position, gdk_device_get_position or
gdk_device_warp. Window-relative and screen-relative coordinate spaces are
retained; missing window/device queries initialize outputs rather than leaving
them undefined. No device references are owned by these helpers.

All four targeted objects were attempted with strict diagnostics. Pointer API
errors are gone; other errors still prevent those objects compiling. The next
gtkhid-main diagnostic is gdk_window_process_all_updates, followed by gdk_beep,
GtkAlignment and incomplete data initializers. Log: `build-gtk3/after-pointer.log`.
Pointer movement/runtime behavior is not yet tested.

Callback diagnostic cycle: a serial `make -k libgtk.a` with the compiler error
count limit disabled exposed 238 unused inputs across 15 GTK files and one
DRC renderer callback const mismatch. `tools/callback_conversion.py` applies
the compiler-confirmed parameter list in `callback_parameters.json`: 237 API
inputs retain their original types and receive G_GNUC_UNUSED. Parameters are
checked against the function body before annotation; no warning class is disabled.
These annotations do not implement existing stub hooks such as calibration.

The remaining input is a GLib prepare callback output: it now writes timeout=-1.
GSourceFuncs uses explicit designated fields, and the DRC get_size callback
accepts the GTK3 const rectangle. The earlier GType converter's duplicated
separator after instance_init comments was also corrected in the rule and all
matching source records.

The final strict serial build reports zero unused-parameter, incompatible
function-pointer or initializer-separator errors. Pinout-preview now compiles
into an arm64 object. The GTK archive still fails on other deprecated APIs,
old key constants, signedness and other diagnostics; runtime remains untested.
Logs: `build-gtk3/callback-inventory-serial.log` and
`build-gtk3/after-callbacks-final.log`. Repeating the callback converter proposes
no additional changes.

Dialog-layout cycle: `tools/dialog_layout_conversion.py` updates all eight
matching GTK source files. GtkGrid replaces table construction and spacing;
`pcb-grid.h` places children with explicit boundaries, expansion, fill and
padding. Label alignment uses GtkLabel APIs. OK/Cancel dialog labels are text,
and their button constructors use mnemonic labels. No matching legacy APIs
remain in C source, and a second converter preview proposes no changes.

The routing-selector arm64 object now compiles with strict flags; together with
the existing main-menu and layer-selector objects this clears the focused menu
selector compilation targets. Runtime layout/interaction remains untested.
Log: `build-gtk3/route-selector-grid.log`.

The seven other affected object targets were attempted separately with make -k.
They still stop on existing callback warnings, other deprecated stock/window
APIs, signedness or old GDK key names. See
`build-gtk3/dialog-layout-objects.log`; no complete GTK library/executable is
claimed. Further conversion should start with a project-wide pattern search of
those diagnostics.

Selector action cycle: `tools/selector_actions_conversion.py` converts both
selectors. Layer visibility/selection uses owned check-menu items and direct
callbacks; routing uses direct menu callbacks and radio-button callbacks with
explicit synchronization. Keyboard shortcuts bind to widgets. Headers retain
their existing typed public contracts. Neither implementation retains GtkAction,
GtkActivatable or a deprecation exemption. Re-running the converter proposes no
additional edits.

Targeted layer-selector compilation passes with strict diagnostics and produces
an arm64 object. Routing compilation still stops on its existing GtkTable,
GtkMisc alignment and stock button labels; no routing object or working GUI is
claimed. Logs: `build-gtk3/selectors-direct.log` and
`build-gtk3/route-selector-all-errors.log` (the latter disables only the diagnostic
count limit, leaving warnings as errors). The next project-wide match list is
`scratch/selector-next-layout-files.txt`. Runtime behavior remains unverified.

Main-menu action cycle: `tools/menu_actions_conversion.py` converts the main
menu implementation, its header and gui-top-window callers together. Menu items
now activate typed C callbacks directly, with widget accelerators, check states
and sensitivity. Signal IDs block callbacks during state synchronization, avoiding
function-pointer-to-void-pointer casts. The main-menu arm64 object compiles under
the strict GTK flags: `build-gtk3/main-menu-direct.log`. The caller object still
fails on existing unused parameters and stock/widget APIs:
`build-gtk3/top-window-direct-menu.log`. No working GUI behavior is claimed.

The gschem icon assets and original provenance README are copied under
`src/icons/bundled/`. `make -C upgrade_data/scratch/build-gtk3/src/icons
pcb-icons.gresource` successfully compiles their manifest, using PCB's own
resource prefix. Runtime embedding/registration and toolbar consumers remain
pending. Layer-selector and route-style-selector action conversions also remain
pending; their shared selection behavior requires different rules from main-menu
resource dispatch.

GType cycle: `tools/gtype_conversion.py` converted seven registration records
across main-menu, route-style-selector, pinout-preview, trackball, DRC window
and library window. Exact GLib-signature adapters call the existing typed class
and instance initializers; value_table is explicitly NULL. The project-wide
search now finds no GClassInitFunc/GInstanceInitFunc casts in source files.
All six object targets were attempted with make -k; compilation still fails on
other strict diagnostics, so these objects are not claimed as compiled.
Log: `upgrade_data/scratch/build-gtk3/gtype-objects.log`. Menu action conversion
remains pending; no new deprecation exemption was introduced in this cycle.

Tearoff cycle completed: `tools/menu_conversion.py` previewed and removed the
GTK tearoff item and both LessTif tearoff-enable statements. Project-wide search
finds no remaining tearoff API calls. The targeted command was:

```sh
make -C upgrade_data/scratch/build-gtk3/src -j1 hid/gtk/libgtk_a-ghid-main-menu.o
```

It still fails on GtkAction deprecations and manual GType registration/callback
warnings; no main-menu object is claimed. LessTif is not configured, so its
source edits have not been compiled. Build log:
`upgrade_data/scratch/build-gtk3/main-menu-after-tearoff.log`.

1. Remove tearoff creation and associated settings across the matching set.
2. Replace deprecated action machinery in main-menu, layer-selector and
   route-style-selector, updating their headers and gui-top-window callers
   together. Retain explicit callback types, toggle state and keyboard shortcuts.
3. Check remaining context menus in coord-entry, dialogs and gtkhid-main for
   the same patterns. Ordinary GTK3 menu widgets need not be replaced.
4. Implement each confirmed pattern in Python, preview all matching edits, then
   apply and compile the set. Compilation is the current acceptance target.

The temporary GtkAction deprecation exemption in ghid-layer-selector.c must be
removed when its action conversion is complete.
