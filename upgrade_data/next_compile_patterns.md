# Next compile patterns

Unified GTK3 scales and linked executable (2026-10-21):

- Whole-project C/header search found only two legacy scale constructors, both
  in gui-utils.c. Ruleset 12 converts them to gtk_scale_new with horizontal or
  vertical orientation, preserving adjustments and slider configuration.
- gui-utils.o and libgtk.a build under strict GTK flags. The GTK executable
  then links successfully: scratch/build-gtk3/src/pcb, Mach-O arm64, -g3 -O0.
  Logs: scratch/build-gtk3/scale-conversion.log, after-scales.log and
  after-gtk-archive.log. Linker warns about duplicate glib/intl libraries.
- --version returns PCB version 0.1.1-20261020, but startup also reports macOS
  display/service warnings, inability to create ~/.pcb in the sandbox, and
  gtk_window_resize width > 0. Those are runtime follow-ups; GUI operation is
  not validated. There is no remaining compile/link blocker in this target.

GTK3 canvas background policy (2026-10-21):

- Table rule `drawing-area-application-background` replaces the sole legacy
  gdk_window_set_back_pixmap(window, NULL, FALSE) call with application-paintable
  drawing-area policy. That old NULL call disabled background clearing; it did
  not allocate or swap drawing buffers. The Cairo backing surface and draw
  callback presentation remain intact.
- gui-top-window.o now compiles with -Wall -Wextra -Werror -Wpedantic.
  Log: `scratch/build-gtk3/background-policy.log`. No GUI runtime claim yet.
- Resumed archive build stops in gui-utils.c:472/474 on gtk_hscale_new and
  gtk_vscale_new; modern replacement is gtk_scale_new with explicit orientation.

Embedded XPM stream conversion (2026-10-21):

- Keep all fourteen mode images and the application icon compiled into the
  executable. `pcb-xpm.h` serializes their explicit-length arrays as XPM text
  and decodes through GMemoryInputStream / gdk_pixbuf_new_from_stream, with
  error reporting and owned-buffer cleanup. No theme lookup or external icon
  file is introduced. The default-icon pixbuf now releases its caller reference.
- One-time `tools/xpm_stream_conversion.py` previews/applies the reviewed set;
  it is not part of bootstrap. No artwork format conversion was necessary.
- A headless strict C fixture decoded all 15 embedded images successfully.
  Targeted top-window compilation has no XPM errors; the remaining stop is
  removed `gdk_window_set_back_pixmap` at drawing-area setup.
  Logs: `scratch/build-gtk3/xpm-stream.log`; fixture: `scratch/xpm-stream-test.c`.

Application-owned top-bar appearance (2026-10-21):

- `tools/topbar_appearance_conversion.py` replaces the reviewed GtkStyle copying
  block with `PcbTopbarAppearance`: explicit GdkRGBA background, foreground and
  border values plus guint padding. A window-local GTK3 CSS adapter applies them
  to the existing top-bar widgets. No temporary probe widgets or desktop theme
  notification handlers remain. The converter previews by default and is
  idempotent; it is not a build dependency.
- Targeted strict compilation reports no errors in the replacement. The object
  still stops on two XPM loaders and removed `gdk_window_set_back_pixmap`.
  Log: `scratch/build-gtk3/topbar-appearance.log`. Visual behavior has not yet
  been tested; no complete GTK executable is claimed.

Typed line-cap state and sentinel audit (2026-10-21):

- `tools/render_cache_conversion.py` converts PS, EPS and Gerber line-cap caches
  to EndCapStyle plus bool lastcap_valid. Reset/first-use/cache-hit behavior is
  explicit; the enum gains no invalid drawing style. PNG's unused cap cache is
  removed. Other cache/index sentinels are not silently reinterpreted.
- Batch rebuild succeeds; the Gerber object also compiles. Exporter cap enum/int
  comparison warnings are gone. PNG is unconfigured and its source edit remains
  uncompiled. The configured core/exporter warning policy is not yet fully strict.
- Before/after PS export of the same two-resistor board has byte-identical top
  copper output and the same cap/join command sequence on all pages. Whole-file
  differences are generated dates, including fabrication-page text strokes.
  Log: `scratch/cap-export-comparison.log`.
- `tools/sentinel_audit.py` scans inventoried C/C++ source/header code, excluding
  comments and literals, and writes `sentinel_candidates.json`. It records 918
  occurrences: 35 cache/state candidates, 27 pointer candidates, 70 return-value
  contracts, 9 index/not-found candidates, and 777 arithmetic/other constants.
  Categories are heuristic prompts for review, not verdicts or safe auto-fixes.
  No API error convention, third-party implementation or index contract is
  changed by the audit.

Build logs: `scratch/build-batch/render-cache-build.log` and
`gerber-cache-object.log`. Repeating the cache converter proposes no edits.
Remaining state/pointer candidates require further focused conversion cycles;
no project-wide elimination of -1 is claimed.

Signed layer-index cycle completed (2026-10-21): the two netlist warnings were
expansions of shared layer macros, not local netlist counters. The reviewed
`tools/layer_loop_conversion.py` converts sixteen layer counters to int, matching
the signed layer bounds. Cardinal remains the object-count type; no blanket
casts or warning exemptions were added. The netlist object now compiles strictly.

A fresh make -k pass across the configured GTK archive reports zero sign-compare
diagnostics. Other GTK errors remain. A broader batch rebuild with explicit
-Wsign-compare succeeds but reveals 204 core/exporter warnings, recorded in
`signedness_candidates.json` by source location and coarse category. These are
review candidates, not approved global type substitutions. In particular shared
GROUP_LOOP layer-ID comparisons, array lengths and genuinely signed values need
separate decisions. Generated lexer cases must be fixed at their source/generator
boundary. This audit does not claim a warning-clean core.

Logs: `scratch/build-gtk3/after-layer-loops.log`, `scratch/build-gtk3/signedness-audit.log`,
`scratch/build-batch/signedness-audit.log`. Full GUI runtime remains untested.

Legacy GDK lock set completed (2026-10-21): project-wide source search found one
ENTER/LEAVE pair, around the coordinate-picking nested GMainLoop in gui-misc.
No worker creation or global GDK lock initialization is present in this checkout.
These calls are obsolete GUI-lock operations, not thread creation/support.

The remove-legacy-gdk-enter rule was applied first; targeted compilation then
reported only LEAVE. The remove-legacy-gdk-leave rule was applied second;
gui-misc now compiles strictly into an arm64 object. No threading libraries,
GLib sources, timers, callbacks or nested event-loop behavior were removed.
No new worker threads are implemented by this migration. The intermediate
unpaired state was compiled only, never run or committed.

Logs: `scratch/thread-lock-candidates.txt`, `scratch/build-gtk3/after-thread-enter.log`,
`after-thread-leave.log`, `next-after-thread-locks.log`. The next library stop is
two Cardinal/int comparisons in gui-netlist-window.c. Full GUI runtime remains
untested and no executable is linked yet.

Library-window set completed (2026-10-21), one compiled item at a time:

1. `tools/reverse_loop_conversion.py` converts twelve reviewed Cardinal reverse
   loops: nine shared macros plus drill and rats code. Countdown conditions retain
   reverse order without signed -1 comparisons. Drill's post-search check uses
   its actual count. The separate signed find.c loop is not converted. A strict
   fixture using the real MENU_LOOP/ENTRY_LOOP macros passes for 0, 1 and 3 entries.
   Batch rebuild passes (`scratch/build-batch/reverse-loops-build.log`).
2. Reviewed GTK rules convert library box/pane object construction to GtkBox and
   GtkPaned with explicit orientations. Trackball class/instance declarations
   and registration also use GtkBox, with vertical orientation in its constructor.
   Trackball/GL remain disabled and uncompiled.
3. `tools/dialog_layout_conversion.py` converts the single object-constructed
   alignment wrapper to a vertical box with the same five-pixel margins.

The library-window arm64 object compiles with strict flags. All three converter
previews are unchanged on repeat. Logs: `scratch/build-gtk3/after-reverse-loops.log`,
`after-widget-types.log`, `after-preview-alignment.log`. The next library stop is
the obsolete gdk_threads_leave/enter pair in gui-misc.c (`next-after-library.log`).
No full GTK executable or runtime GUI validation is claimed.

Dialog blocker set completed (2026-10-21):

- `tools/dialog_stock_conversion.py` removes the remaining stock constants and
  stock-image calls across dialog, library and main-window sources. Controls use
  explicit Save/Close/Clear text; reload/warning decoration is removed. Response
  IDs, default save response and callbacks are retained. The sole alternative
  button-order call is removed, retaining the declared application order.
- The following DRC stop exposed a wrongly typed class initializer. The reviewed
  drc-class-initializer-type rule corrects it to GhidDrcViolationClass, matching
  its registered class and adapter. The three tree-view row-stripe hints are
  removed through desktop_hints_conversion.py across DRC/netlist views.

Dialog and DRC objects compile strictly into arm64 objects. No stock-image,
stock-constant, alternative-button-order or row-stripe-hint calls remain in C
sources. Logs: `scratch/build-gtk3/after-dialog-stock.log`,
`after-drc-class-hints.log` and `next-after-drc.log`. Runtime UI remains untested;
the full GUI executable is not linked.

Preferences cycle completed (2026-10-20): both explicit scrolled-window viewport
insertion sites use GTK3 gtk_container_add, which supplies a viewport for the
grid/box children. Two reviewed layer-entry indices in gui-config use Cardinal,
matching LayerGroupType.Number; signed group/layer IDs retain their types. Broader
count-loop candidates are recorded in `scratch/layer-entry-loop-candidates.txt`
and require individual review rather than global casts.

Both GtkArrow controls are replaced by text Up/Down buttons through reviewed
rules, preserving their layer-operation callbacks. No icon lookup is introduced.
The preferences object now compiles strictly into an arm64 object. Logs:
`scratch/build-gtk3/after-viewport.log`, `after-layer-entry-indices.log`,
`after-layer-buttons.log` and `next-after-config.log`. Runtime layout and layer
operations remain untested.

Desktop-hint cycle completed (2026-10-20): the user explicitly excludes optional
desktop policy integration. `tools/desktop_hints_conversion.py` removes eleven
wmclass calls across ten files, then the two pager/taskbar hints on the progress
dialog. Titles, parent relationships and modality remain. Project-wide source
search finds none of these calls. GTK main and dialog-print objects compile
strictly; command-window remains blocked by old key/button APIs and an array
loop signedness warning. Logs: `scratch/build-gtk3/after-desktop-hints.log` and
`after-desktop-policy.log`. This does not claim that all remaining icon/theme
consumers have been migrated; bundled resource registration is still pending.

Final gtkhid-main API cycles completed (2026-10-20):

- `tools/benchmark_conversion.py` replaces process_updates with synchronous
  GTK drawing into an owned Cairo image surface after backing-image regeneration.
  Monotonic timing measures ten seconds of completed offscreen redraws, explicitly
  reported as such; this is not display presentation throughput. Cairo objects
  are released, including on allocation failure. The targeted compile then had
  only the popup deprecation left.
- The popup-event-anchor rule in `gtk_api_migrations.json` uses the current event
  for pointer placement on mouse/touch input and widget anchoring otherwise.
  The owned event copy is freed after the popup request.

`make -C scratch/build-gtk3/src hid/gtk/libgtk_a-gtkhid-main.o` (with the actual
upgrade_data prefix) now succeeds using the configured strict flags and produces
an arm64 Mach-O object. Neither deprecated call remains in C/header source.
Logs: `scratch/build-gtk3/after-benchmark.log` and `after-popup.log`. Benchmark
performance and popup interaction remain untested; the full GUI is not linked.

Numeric boundary cycles completed (2026-10-20):

- `tools/hid_bounds_conversion.py` introduces HID_Attr_Bound with explicit int,
  double and Coord members. All attribute initializers select the member by HID
  type, and the four GTK bounds consumers read the corresponding member. This
  preserves fractional real minima such as 0.5 and 0.01 without losing precision
  in 64-bit coordinate limits. Numeric values and existing zero-bound conventions
  are retained; input validation policy is not redesigned.
- `tools/hid_flags_conversion.py` makes the five boolean HID bitfields unsigned.
  Searches found boolean tests/assignments, not signed sentinel semantics.

Both scripts were previewed and applied separately, with targeted strict builds
after each. Fraction truncation and one-bit sign diagnostics are gone. Main GTK
object now has only benchmark process_updates and popup-menu deprecation errors.
The dialog consumer retains unrelated old window/combo APIs. Batch rebuild passes
and reports version 0.1.1-20261020. The shared HID layout changed, so linked
consumers must be rebuilt together; this does not preserve the old plugin ABI.

A compiled fixture against the actual headers checks 0.5/0.01 real limits,
INT_MIN/MAX, positive/negative COORD_MAX, and zero/one round trips for all five
flags. It passes under -Wall -Wextra -Werror -Wpedantic. Fixture/source/log live
under `scratch/hid-numeric-check*`. Build logs: `scratch/build-gtk3/after-hid-bounds.log`,
`after-hid-flags.log`, and `scratch/build-batch/hid-numeric-build.log`.

Evidence: `scratch/build-gtk3/after-alignment.log`, 2026-10-20.

Initializer cycle completed: `tools/hid_initializer_conversion.py` converted
292 explicit records in 49 tables across 43 source files. HID_Action,
HID_Attribute and nested default HID_Attr_Val fields are designated; existing
values, callbacks, table order, comments and preprocessor directives are retained.
The converter checks value-token equivalence before proposing each file. Omitted
members retain C's zero-initialization. A repeated preview proposes no changes.

The strict GTK main object no longer reports missing-field initializers. Its
remaining six diagnostics concern two fractional bounds, two signed bitfields,
benchmark redraw and popup placement. The batch build passes and its executable
reports version 0.1.1-20261020. Logs: `scratch/build-gtk3/after-hid-initializers.log`
and `scratch/build-batch/hid-initializers-build.log`. Unconfigured exporter and
LessTif source conversions remain uncompiled. No runtime GUI validation is claimed.

1. **Positional HID initializers.** Short HID_Action entries omit description
   and syntax; HID_Attribute entries omit hash and nested default_val fields.
   Convert reviewed tables to designated fields, retaining callback types.
   Candidate tables are listed in `scratch/hid-initializer-candidates.json`;
   matches are candidates, not confirmed errors.
2. **Fractional real limits stored in integer fields.** Printer calibration
   uses minimum 0.5, but HID_Attribute.min_val/max_val are int, so the minimum
   becomes zero. This needs a typed bounds decision and consumer review, not
   zero-padding the initializer or casting away the warning.
3. **Signed single-bit flags.** Five HID fields are declared char :1; assigning
   TRUE can become -1. Review unsigned flag storage and all users together.
4. **Benchmark drawing.** One gdk_window_process_updates call remains. Its
   replacement must still measure rendering rather than queued redraw requests.
5. **Popup placement.** One gtk_menu_popup call remains; choose the GTK3 anchor
   API based on its event/keyboard invocation context.

The first three patterns share a cause: older declarations and positional
initializers no longer express the intended types clearly. Handle each as a
separate reviewed Python conversion followed by targeted compilation.
