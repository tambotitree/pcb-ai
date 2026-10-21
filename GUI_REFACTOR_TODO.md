# PCB-ai GUI refactor list

This is a rough implementation list, not a detailed inventory. Work proceeds
in consultation with John Ryan. Prefer repeatable conversion rules for repeated
patterns, followed by compilation and focused behavior checks.

## What worked in gschem

- Explicit C action records and typed callback declarations replaced the old
  macro-driven action implementation. The generator checked that action bodies
  retained their behavior.
- Action registration/activation became Guile-free; menu layout still reads
  Scheme definitions for user configuration and Scheme-defined Help actions.
  We did not remove Scheme everywhere.
- Icons were embedded as GResources and initialized before constructing menus.
  Aliases cover stock names and directional variants. Menus/actions must still
  exist when artwork is missing.
- Appearance was fixed to Adwaita/light/Sans through a small shared policy.
  Filesystem icon-theme searches were removed; the user's desktop was unchanged.
  Drawing colors remain independent of widget appearance.
- Callback signatures, object ownership, viewport transforms, redraw bounds,
  previews, grid/snap and endpoint behavior were repaired and checked.

Reference implementations in the companion gEDA-ai tree:
`gschem/src/actions.c`, `gschem/src/actions.h`,
`gschem/src/gschem_action.c`, `gschem/src/x_menus.c`,
`gschem/src/gschem_signal.c`, `gschem/src/gschem_appearance.c`,
`gschem/src/gschem_icons.c`, `gschem/src/gschem_page_view.c`,
and `gschem/icon-theme/README`. Reuse verified ideas rather than copying entire
modules whose schematic-specific assumptions differ from PCB.

## PCB starting point

- [x] Batch interface and PostScript exporter compile and run.
- [x] Two-resistor load/save/reopen, connectivity and PS/PDF smoke test passes.
- [x] Mechanical conversion replaced 72 basic GTK constructors in 13 files.
- [ ] Full GTK3 GUI build and runtime behavior remain unverified.

PCB already has a C action registry and resource-based menu descriptions.
Keep that useful boundary. No new Scheme, JSON plugin system or second action
framework is needed to get a working window.

## Automation first

For repeated changes, implement the Python conversion rule before editing the
production files. Keep separate scripts for distinct conversion types. Use the
existing JSON inventory for file selection, source hashes, versions and history.
No API call or model is required for deterministic mechanical conversions.

| Area | Automated pass | Boundary requiring a decision or review |
| --- | --- | --- |
| Function signatures | Extend `tools/c_signature_conversion.py`; compare generated definitions/declarations with headers | Complex declarators and old scalar promotion semantics |
| GTK widgets | Extend `tools/c_conversion.py` with narrow, token-aware rules | Container semantics, custom widget inheritance and ownership |
| Action callbacks | Add a dedicated action-adapter conversion script when signatures are agreed | Preserve command IDs, toggle state, context and accelerators |
| Icons | Add an icon inventory/manifest generator using actual C/resource requests | Choose artwork, aliases and attribution; missing art cannot remove actions |
| Appearance | Generate repetitive settings/initialization edits after policy is agreed | Keep widget appearance independent of copper/rendering colors |
| Drawing and events | Add dedicated conversion passes for established API patterns | Choose backing storage and overlay design; preserve transforms and behavior |
| Verification | Script fresh compilation and the load/save/connectivity/export fixtures | Interactive routing and physical printing still need direct checks |

Each new converter must:

- [ ] Preview by default and write only with explicit `--apply`.
- [ ] Select inventoried files and reject stale source hashes.
- [ ] Preserve comments, strings and unrelated function bodies.
- [ ] Refuse ambiguous patterns rather than guess a type or object role.
- [ ] Retain original bytes and a machine-readable report in ignored scratch.
- [ ] Record converter identity/version and before/after hashes in the inventory.
- [ ] Produce no further changes when rerun on converted input.
- [ ] Check representative changed and deliberately unchanged examples before use.
- [ ] Compile affected code and leave changes marked unverified until checks pass.

Existing tools provide the selection/reporting framework. Extend or split it as
needed rather than repeatedly applying handwritten substitutions to source files.
Human review is for approving the rule and checking behavior, not manually
repeating a transformation across dozens of files.

## 1. Establish the GTK3 build boundary

- [ ] Keep the working batch/PostScript build directory as a separate baseline.
- [x] Add a separate experimental GTK3 build directory and confirm GTK3 library
  selection. Source compilation is expected to fail during migration; no working
  GUI is claimed. The legacy gtkglext path is explicitly rejected.
- [ ] Preserve the `HID` rendering/action interfaces used by core and exporters.
- [ ] Keep debug symbols. During the agreed rough build pass warnings remain
  nonfatal; restore strict diagnostics as each migrated area is brought under
  control.

Primary files: `configure.ac`, `src/Makefile.am`, `src/hid.h`,
`src/hid/gtk/gtkhid.h`, `src/hid/gtk/gui.h`.

## 2. Type and simplify the GUI action adapters

- [ ] Use the existing PCB action names and core callbacks as the command source.
- [ ] Give menu, toolbar, toggle and accelerator adapters explicit signatures.
  Keep generic GTK callback casts at the GTK registration boundary.
- [ ] Replace legacy action/widget machinery incrementally where it blocks the
  port. Preserve enabled/checked state, accelerators and layer/route-style menus.
- [ ] Keep menu descriptions in the existing `.res` files; avoid an unnecessary
  menu-format conversion.
- [ ] Ensure missing icons cannot stop menu construction or command activation.
- [ ] Resolve the batch `PointCursor` warning at the GUI/HID boundary without
  inventing a fake engineering action.

Primary files: `src/hid/gtk/ghid-main-menu.[ch]`,
`src/hid/gtk/gui-top-window.c`, `src/hid/gtk/gtkhid-main.c`,
`src/action.c`, `src/hid/common/actions.c`, `src/gpcb-menu.res.in`.

## 3. Bundle icons and fix appearance

- [ ] Adopt the small fixed-appearance approach from gschem, using explicit
  constants and initialization before creating widgets.
- [ ] Inventory actual PCB toolbar, dialog and application icon requests.
- [ ] Reuse suitable original PCB/gEDA artwork with its license and attribution.
- [ ] Embed the artwork with GResources; cover required stock-name aliases and
  directional variants. Keep normal builds independent of network downloads.
- [ ] Verify icons with filesystem icon-theme searches disabled.
- [ ] Keep optional desktop/MIME installation separate from editor startup.
  Desktop integration must not be required for the canvas to work.

Primary files: `src/hid/gtk/gui-top-window.c`,
`src/hid/gtk/gui-icons-misc.data`, `src/hid/gtk/gui-icons-mode-buttons.data`,
`src/hid/gtk/pcb.rc`, `data/Makefile.am`, `src/Makefile.am`.

## 4. Replace obsolete drawing infrastructure

- [ ] Inspect the GDK and GL backend contracts before selecting the first GTK3
  implementation. Proposed first target: a Cairo drawing path without gtkglext;
  confirm that decision before implementation.
- [ ] Replace `GdkGC`, `GdkPixmap`, XOR drawing and GTK2 expose handlers with
  GTK3-compatible drawing, backing storage and temporary overlays.
- [ ] Keep completed board geometry distinct from cursor, selection and routing
  previews so old temporary marks can be removed reliably.
- [ ] Preserve PCB world/device conversions, zoom, pan, layer orientation,
  clipping, copper clearances and rendering order.
- [ ] Verify cover/uncover, resize and full redraw reproduce the same board.
- [ ] Port footprint previews using the same verified transform conventions.

Primary files: `src/hid/gtk/gtkhid-gdk.c`, `src/hid/gtk/gtkhid-gl.c`,
`src/hid/gtk/gui-pinout-preview.[ch]`, `src/hid/gtk/gui-top-window.c`,
`src/hid/common/hidgl.c`, `src/hid/common/draw_helpers.c`.

## 5. Port window, widget and object lifetimes

- [ ] Create a usable main window, menu, status area and drawing canvas first.
- [ ] Replace obsolete widget-field access and GTK2 widget APIs with GTK3
  accessors or narrow adapters.
- [ ] Check ownership and reference sinking for parented/unparented widgets;
  disconnect callbacks before destroying their contexts.
- [ ] Give useful initial sizes to the main window and essential dialogs.
- [ ] Bring open/save dialogs and footprint selection online before secondary
  preferences, specialized views or visual polish.

Primary files: `src/hid/gtk/gui-top-window.c`, `src/hid/gtk/gui-dialog.c`,
`src/hid/gtk/gui-library-window.c`, `src/hid/gtk/gui-utils.c`,
`src/hid/gtk/ghid-main-menu.c` (contains an unfinished finalize TODO).

## 6. Port pointer events and engineering interaction

- [ ] Use explicit event callback types and keep coordinate conversion in a
  consistent place.
- [ ] Verify grid alignment, snapping, pad/via endpoints, selection and movement.
- [ ] Verify route previews erase correctly, including cancellation and redraw.
- [ ] Connect a trace to the actual copper endpoints; validate connectivity
  rather than relying on visual proximity.
- [ ] Check undo/redo and save/reload after interaction.

Primary files: `src/hid/gtk/gui-output-events.c`, `src/hid/gtk/gui-misc.c`,
`src/crosshair.c`, `src/line.c`, `src/move.c`, `src/undo.c`.

## 7. Reuse the two-resistor acceptance path

- [ ] Open the existing two-resistor layout in the GUI.
- [ ] Place the components, draw the trace interactively and check connectivity.
- [ ] Save, close and reopen; compare the net and board contents.
- [ ] Export copper without changing the proven batch/PostScript path.
- [ ] Move from test-only to physically verified footprints, then check deliberate
  mirroring and a laser print at Actual Size before toner transfer.

The batch test is a regression baseline, not evidence that GUI routing or physical
manufacture works. Keep the trace-removed negative control.

## Decisions and next step

Next proposed step: establish the GTK3 build boundary in a separate directory,
then let compiler errors drive small migration passes. Agree on the initial
drawing backend before replacing rendering code. No GUI architecture change is
implemented by this document.

Notes:

- 2026-10-20: Compared actual gschem action, icon and appearance code with PCB's
  C actions, resource menus and GDK/gtkglext backends. Recorded this refactor list.

### Cycle 1 — 2026-10-20

Added and applied `tools/gtk3_build_conversion.py`: exact configure dependency
rewrites, preview/report/history through the shared runner, and a guard against
linking GTK2 gtkglext into GTK3. Bootstrap and GTK3 configure passed. Converter
idempotence passed. The generated Makefile selects GTK3, not GTK2.

Separate build: `upgrade_data/scratch/build-gtk3`, debug enabled, GL disabled,
PostScript exporter retained, nonfatal warnings. Compilation fails in gui.h on
removed GTK2 types: GtkObject, GdkPixmap, GdkDrawable and GdkColormap. Clear serial
build output is in `first-blocker.log`; initial parallel output in `build-output.log`.

Next bounded pass: type the adjustment fields as GtkAdjustment and update their
constructors/casts using a dedicated rule. The drawing types are a separate,
coordinated rendering migration; do not substitute generic pointers for them.
The working batch build has not been converted into a GUI build.

### Cycle 2 — 2026-10-20: scoped strict diagnostics and first cleanup

Enabled `libgtk_a_CFLAGS = $(AM_CFLAGS) -Wall -Wextra -Werror -Wpedantic`.
These are per-library flags for GTK GUI sources, not global CFLAGS or configure
probe flags. Build through `make -C src -j1 libgtk.a` in the GTK3 build directory.

The diagnostics first exposed a redundant semicolon in ANYOBJECTFIELDS, expanded
through shared structs. Added `tools/struct_macro_conversion.py`, previewed and
applied its narrow rule to src/global.h. Field names/types/order are unchanged.
A compiled fixture compared original/converted sizes and offsets; identical.
The converted macros pass strict diagnostics, and the converter is idempotent.
A rebuilt GTK object no longer reports the redundant struct-semicolon errors.

Current blockers: GTimeVal, removed GTK2 drawing/object types, deprecated style,
color and Cairo-window APIs, and unused callback parameters. Do not suppress
these warnings or replace typed fields with generic pointers. Next bounded pass
can address GtkAdjustment declarations and their associated constructors/casts.
Logs: `upgrade_data/scratch/build-gtk3/strict-first-blocker.log` and
`strict-after-macro.log`. No further GTK source migration was applied in this cycle.

### Cycle 3 — 2026-10-20: table-driven GTK API rules

Added `tools/gtk_api_conversion.py`. RULES records pattern, replacement, file
scope and rationale. Four approved rules type the adjustment fields, update the
known dialog GObject casts, and replace horizontal/vertical scrollbar
constructors. The table also reports GTimeVal, style, color and context-lifecycle
patterns as needs_rule without changing them. No blind API-name substitution.

Four regression tests pass: field/scope handling, preservation of strings and
comments (including comments within expressions), scrollbar arguments and
idempotence, and deferred changes. Applied the approved rules to gui.h,
gui-utils.c and gui-top-window.c. A strict GTK rebuild confirms the GtkObject
field error is cleared. GTimeVal/drawing types and cell-renderer API/callback
warnings remain. These three production files are not yet fully compilable.
Log: `upgrade_data/scratch/build-gtk3/strict-after-gtk-api-table.log`.

### Cycle 4 — 2026-10-20: timestamps using the gschem precedent

Inspected gschem_change_notification.c and x_highlevel.c: gschem uses
struct timespec, explicit known-state and stat member checks, not GDateTime.
Applied the one-time timestamp_conversion.py pass to gui.h,
gui-top-window.c and configure.ac. Future source is authoritative; no normal
build runs conversion scripts. Windows-specific work is deferred until that port.

GTimeVal and deprecated GFileInfo timestamp calls are gone. The converted
functions read stat directly, compare seconds/nanoseconds and track an explicit
known timestamp. Configure detects st_mtim/st_mtimespec and selects macOS's
st_mtimespec here. No old GTimeVal compatibility path is retained.

Checks: bootstrap/configure passed; strict GTK rebuild clears GTimeVal errors.
Compiled the actual converted functions in a small fixture with strict warnings;
unknown, unchanged, nanosecond-only update, repeated observation, older/newer,
epoch zero and missing-file cases passed. Drawing types and cell-renderer APIs
remain blocked. Logs: timestamp-configure.log and strict-after-timestamps.log
under upgrade_data/scratch/build-gtk3. The fixture is timestamp-behavior.c in
scratch. No further conversion is run automatically after this migration.

### 2026-10-20 — Two evidence sources in the conversion table

Moved GTK API rules to `upgrade_data/gtk_api_migrations.json`, read directly by
`tools/gtk_api_conversion.py`. Rows retain old API, compiler suggestion, selected
target, gschem example locations, rationale, pattern, scope and conversion state.
Only approved replacement expressions are executable; coordinated changes stay
report-only. Normal builds do not run this one-time migration tooling.

Inspected gschem's swatch renderers, x_color.c, style-context queries and x_stroke.c.
Important contextual distinction: GTK3 cell renderers receive a borrowed cairo_t
from GTK; they should not blindly replace gdk_cairo_create with begin_draw_frame.
The latter is the direct-window drawing pattern used in x_stroke.c. Color changes
also require argument-order and 16-bit-to-floating-channel updates together.
The table records those constraints rather than assuming API names are equivalent.
No new production rewrite was applied in this evidence-gathering step.

### Cycle 5 — 2026-10-20: visibility cell renderer

Created a guarded one-time visibility_renderer_conversion.py pass for the
reviewed original module, preserving its copyright/license header. Applied the
coordinated GTK3 signature, borrowed Cairo context, RGBA argument/channel changes,
style-context border measurements and explicit unused callback arguments.
Retained active/inactive shapes, prelit shading and toggled-path behavior.
G_DEFINE_TYPE provides the typed GObject registration bridge; finalize releases
the owned color string. Invalid/unset colors now fall back deterministically.
Removed unnecessary gtkhid/gui includes from this standalone renderer.

The staged source compiled under strict GTK3 diagnostics before application.
The real production object then built with the strict GTK library flags. A
compiled GTK behavior fixture checks active/inactive/prelit pixels, clipping,
caller Cairo context/graphics/path preservation, invalid/unset-color fallback,
property access, emitted toggle path and finalization. All passed, and the three
rendered swatches were visually inspected. The sandbox emits macOS CVDisplayLink
and XPC initialization messages before testing; no fixture assertion failed.

Run `python3 tools/test_visibility_renderer.py` to check the converted module;
this compiles/tests source directly and does not rerun conversion. Scratch PNGs
are under visibility-renderer-test. Production build log: build-gtk3/
strict-after-visibility.log. The next object, ghid-coord-entry.c, remains blocked
by gui.h's old drawing types and its own strict-warning diagnostics. Full GUI
build/runtime are still unverified. Stop this cycle here.
# Drawing conversion compile checkpoint

Layer-selector follow-up: `ghid-layer-selector.c` compiles into an arm64 object.
G_DEFINE_TYPE replaces incompatible initializer casts, and the button callback
now includes signal user data. Required unused callback arguments are explicit.
Temporary exception: this file retains GTK3's deprecated GtkAction family with
G_GNUC_BEGIN/END_IGNORE_DEPRECATIONS; replacing those APIs remains menu migration
debt. Other strict warnings remain errors. The next failing object is
`ghid-main-menu.c` (deprecated actions/tearoff menu and callback warnings).
Log: `upgrade_data/scratch/build-gtk3/strict-after-layer-selector.log`.

Follow-up: `ghid-coord-entry.c` now compiles into an arm64 object with the strict
GTK flags. G_DEFINE_TYPE replaces incompatible GType initializer casts; focus
out has its required event parameter, value-changed returns void, and the shared
unit-parsing helper keeps a typed entry pointer. The library next stops in
`ghid-layer-selector.c` on deprecated GtkAction APIs and callback warnings.
Log: `upgrade_data/scratch/build-gtk3/strict-after-coord-entry.log`.

The immediate goal is compilation only; rendering behavior remains unverified.
The normal GTK3 Makefile successfully compiled `gtkhid-gdk.c` and `pcb-cairo.c`
into arm64 Mach-O objects with `-Wall -Wextra -Werror -Wpedantic` enabled.
The complete `libgtk.a` build still stops in `ghid-coord-entry.c`: unused callback
data, incompatible GType initializer function casts, and an incomplete GTypeInfo
initializer. No GTK executable has been linked from this conversion yet.
Logs: `upgrade_data/scratch/build-gtk3/drawing-objects.log` and
`upgrade_data/scratch/build-gtk3/strict-after-drawing.log`.
