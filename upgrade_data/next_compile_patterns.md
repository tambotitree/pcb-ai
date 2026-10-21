# Next compile patterns

Visible geometry after reviewed board swap (2026-10-21):

- User observed a blank main canvas after Implement. Parsed PCB runtime
  visibility flags were all false: layer On, ElementOn, PinOn, ViaOn, RatOn
  and InvisibleObjectsOn are not saved by the board text serializer.
- Staging now copies the live board's visibility and active drawing mode. Layer
  count changes are rejected for now so the global LayerStack stays valid.
  Existing hidden-layer choices are preserved rather than forcing everything on.
- Earlier image test compared two PNGs but could pass when the new image was
  blank. Strengthened it with visibility assertions and actual copper-pad pixel
  checks through BOTH offscreen HID rendering and the real GTK canvas expose
  callback, after Implement. Review/Undo/Redo and mock-helper flow still pass.
- Full strict GTK build passes. Native test with fatal GTK criticals passes.
  Logs: scratch/ai-visibility-build.log and scratch/ai-visibility-native.log.
  Restart the rebuilt executable; this fix is C, not just a Python helper edit.


Host-generated diffs from numbered replacements (2026-10-21):

- User reached real validation: hand-written model diff context did not match
  the snapshot even after retry. Keep exact matching; do not add fuzzy patching.
- Supply numbered source lines and accept inclusive original-snapshot line ranges
  plus replacement text. Host constructs the unified diff with difflib, then runs
  the same exact patch validation and native review/Implement/Undo pipeline.
- Require the original base hash, valid disjoint ranges, bounded newline-terminated
  text, whitelisted document and all existing font/size checks. Never infer line
  numbers after earlier replacements or accept stale source.
- 32 offline tests pass, including original tabs/context retention, multiple
  ranges with line-count changes, overlap/range/stale/mixed-form rejection.
  Native mock-provider regression uses tagged numbered replacements through the
  real helper and confirms enabled Act, review, Implement and Undo.
  Log: scratch/ai-numbered-patch-native.log.
- User explicitly authorized the live OpenAI gpt-4 fixture test after automatic
  approval review initially rejected transmission. Live request returned one
  validated patch with numbered replacements; all four pad centers stayed at
  +/-15 mil and endpoints became coincident, width 118.11 mil (~3 mm).
  Native batch parser/PostScript export accepted and rendered the candidate.
  Scratch artifacts: ai-live-numbered.pcb/.ps and ai-live-numbered-export.log.
  No live board was changed. This verifies transport/proposal/rendering, not
  design correctness: requested pads overlap and model retained the old small
  mask openings. Broader geometry validation remains a recorded follow-up.


Expose proposal validation errors, bounded correction (2026-10-21):

- The generic main exception handler mislabeled protocol ValueErrors (bad JSON,
  stale base hashes, invalid hunks, operation bounds) as Invalid chat request.
  User wording is not the cause of a rejected model-generated proposal.
- prepare_assist validates the generated proposal and requests one corrected
  response with fixed validator feedback. It retains the original source/hash
  and all strict checks. A second failure reports its specific safe reason and
  confirms no board changes; no retry loop and no automatic implementation.
- Two offline tests verify bad-hash correction, preserved source context, bounded
  retry and visible malformed-JSON error. All 30 offline tests pass.


Explicit proposal section and Act activation (2026-10-21):

- User saw a JSON patch embedded in Markdown/prose, silently treated as chat.
  Agreed delimiter: <pcb-ai-proposal> JSON </pcb-ai-proposal>. Extract exactly
  one section; accept one legacy JSON fence surrounded by prose. Invalid or
  ambiguous structured proposals report errors instead of silently disabling Act.
- Tag alone never authorizes edits: base hash, exact diff, path and native parser
  checks remain. Act opens review; Implement is still the edit boundary.
- Prompt also preserves each pad's distinct center instead of moving every pad
  to the Element origin, as the user's shown proposal incorrectly did.
- 28 offline tests pass. Native mock-provider regression now returns prose plus
  tagged JSON through the actual Python helper and verifies enabled Act, review,
  Implement and Undo. Existing PNG continuation and native edit checks pass.
  Log: scratch/ai-tagged-patch-native.log. No real API request or source board edit.


Correct embedded editing capability guidance (2026-10-21):

- User observed repeated false claims that PCB text cannot change square pads to
  round and that patch review has no visual preview. Current prompt still mixed
  selection-only native-operation wording with general patch capabilities.
- Clarified native operations versus general patches; concrete requested edits
  should produce proposals without another conversational permission loop.
  Implement is the approval boundary. Current system contract follows history,
  so old assistant refusals do not define the current editing capabilities.
- Added pad syntax guidance verified against file.c and draw_helpers.c: remove
  square for round ends; coincident endpoints are needed for a true circle.
  Preserve flags, units, pad/net identity and distinguish mask from copper/drill.
  Describe actual overlap/connection issues, not fictional format limitations.
- All 26 offline protocol/backend tests pass. This is a prompt change, not a
  claim that a live model has now produced a correct patch. No real API call.


Reviewed JSON design patches (2026-10-21):

- Disk-full write failed before editing sources; after reboot 14 GiB was free.
- New design_patch.py stages exact unified diffs in memory with base SHA-256,
  explicit document scope, hunk counts and context checks. No shell patch command.
- JSON patches are encoded through the trusted helper envelope. current.pcb is
  the live unsaved snapshot, not arbitrary model-supplied filesystem access.
- Act now opens a modal review popup; Implement alone applies. Parser failure
  disables Implement. Summary/diff and native before/after viewport are shown.
  The configured FileCommand is disabled while parsing private staged PCB text;
  the original font and filename survive compact-context round trips.
- Native whole-board undo record retains inactive boards and exchanges them on
  Undo/Redo. Redo pruning/undo clearing free inactive boards without RemovePCB's
  recursive ClearUndoList side effect. Existing earlier object undo survives.
- Current scope is the live PCB. Explicit .sch attachment and cross-editor undo,
  model sampling controls, and broader DRC checks are recorded follow-ups.
- Offline tests: 13 backend + 6 visual + 7 exact-patch tests pass. Native GTK
  regression with fatal criticals passes review/cancel/Implement, changed image,
  board Undo/Redo and invalid parser rejection, existing selected pad/via edits,
  menu lifetime, and one asynchronous PNG continuation. No real API calls.
  Both full GTK and batch builds pass. Logs: scratch/ai-patch-build.log,
  scratch/ai-patch-batch-build.log and scratch/ai-patch-native.log.


Menu lifetime and GTK3 startup warnings (2026-10-21):

- User crash report pcb-2026-10-21-123113.ips matches PID 53585 and the reported
  launch. Faulting stack is g_type_check_instance_cast -> menu toggle sync ->
  menu activation callback. User reports no deliberate menu click.
- A native regression reproduced invalid-widget criticals when a cached menu
  item was destroyed and toggle state was refreshed. GHidMainMenu's borrowed
  item list now removes destroyed items; special hotkey bindings similarly clear
  borrowed widget/resource pointers. No attempt is made to cast/test a freed
  object as a validity check.
- Removed GTK2 size-request signal callbacks. Cursor text setters preserve their
  largest preferred width using GTK3 preferred-size and size-request methods.
  Loaded/saved zero window dimensions receive usable defaults, retaining valid
  user sizes for the main window and auxiliary dialogs.
- Full GTK build passes. Native regression with a disposable copy of the user's
  preferences and G_DEBUG=fatal-criticals passes startup, actual AI menu activation,
  destroyed-item toggle refresh, pad/via operations, image exchange and Plan/Act.
  Log: scratch/menu-runtime-regression.log; baseline invalid-widget diagnostics:
  scratch/menu-lifetime-before.log. User must restart the rebuilt executable;
  this does not claim every unrelated runtime failure is fixed.

Explicit Plan / Act interaction (2026-10-21):

- Plan sends questions and develops proposals, never mutating the board. Act
  remains disabled without a validated registered operation. Discussion and
  missing-reference/parameter clarification produce no action. New planning
  requests invalidate the previous proposal; Act retains frozen-state guards.
- Model prompt acknowledges the native operation bridge instead of presenting
  the assistant as guidance-only. It explains the exact supported limitations,
  asks for missing details, and must not claim an edit before Act.
- Full GTK build passes; 13 transport and 6 visual protocol tests pass. Native
  mock-provider regression explicitly verifies Plan leaves pad width unchanged,
  the Act button applies the reviewed operation, and Undo restores it. Existing
  via, selection and automatic-image checks also pass.
  Logs: scratch/ai-plan-act-build.log and scratch/ai-plan-act-native.log.

Shared operation registry and selected-via enlargement (2026-10-21):

- Separate generic selection manifest, operation registry, request-state guards
  and redraw/UI from typed pad and via implementations. No natural-language
  phrase is hard-coded. The registry validates supported names and parameter
  bounds; backend contracts require matching selected object kinds.
- Manifest includes selected component references/bounds, pad identities, plated
  via diameters/drills/layers, unplated holes, and trace layers/endpoints/clearance.
  It is authoritative; images supply visual context, not selection identity.
- Via enlargement changes outer copper diameter only, preserving drills, XY and
  layer spans. Native ChangeObjectSize maintains mask/polygon/index behavior;
  group preflight rejects locked/invalid sets, and one Undo restores all vias.
- Native regression confirms selected vias change, unselected via does not,
  masks/undo work and drill/position/layer spans remain fixed. Existing pad and
  automatic-image exchange checks still pass. Thirteen transport and six visual
  protocol tests pass. Logs: scratch/ai-operations-native.log and
  scratch/ai-operations-build.log.
- Model instructions require asking rather than inventing missing references or
  circuit groups. Creation requires placement and part ID or pinout/footprint.
  General lookup/connectivity operations, component creation and constrained
  routing remain explicit follow-up work, not advertised as implemented.

Visual AI exchange and selected-pad editing (2026-10-21):

- New Send image control attaches a freshly rendered, lossless PNG of the
  current viewport/layers (maximum dimension 1280; encoded PNG bounded to 8 MiB).
  Optional automatic requests perform at most one asynchronous image continuation
  against the same frozen board snapshot; changed boards must be resent.
- assist_protocol.py validates model JSON, then emits a base64/INI envelope.
  Plain prose cannot execute actions. Only scale_selected_pad_width with finite
  10..200 percent is accepted, and only for pads selected before the request.
- Explicit Apply rejects changed board/selection, locked pads and invalid widths;
  native ChangeObjectSize maintains mask geometry, spatial indices and polygon
  updates. Resize is grouped into one undo step. Apply redraws both canvas and
  image preview. This is not a substitute for DRC/manufacturing review.
- New-pad creation over vias, pad length/area scaling and arbitrary object edits
  remain follow-up work; do not advertise them as implemented.
- Full GTK build passes with strict GTK warnings. Thirteen transport tests and
  five visual protocol tests pass. A private native regression plugin confirms
  two selected pad widths/masks change, an unselected pad does not, one undo
  restores both, stale selection is rejected, and PNG pixels change after Apply.
  Initial image test was over-zoomed due to coordinate units; a dimension-based
  viewport makes the real before/after check meaningful.
- Loopback mock-provider integration confirms text -> requested PNG -> typed
  proposal, with exactly two HTTP requests and no external API or real keys.
  Native runner uses only a disposable board copy. Logs: scratch/ai-native-test.log,
  scratch/ai-visual-full-build.log. Provider/model image support remains a user
  choice; disable automatic images for text-only models.
- OpenAI content-part formatting checked against official Chat Completions API:
  https://developers.openai.com/api/reference/resources/chat/subresources/completions/methods/create

AI context-limit fix (2026-10-21):

- Reproduced the gpt-4 HTTP 400 using the two-resistor fixture: OpenAI's safe
  structured code is context_length_exceeded. Native serialization embeds all
  vector font strokes, exhausting this model's context on a tiny board.
- AI snapshot writer now omits only font glyph geometry and marks that omission.
  Normal save/backup paths still include fonts. Labels, components, attributes,
  nets and routed geometry remain in the AI context.
- Equivalent compact fixture is 1,633 bytes versus 31,178 bytes; the same
  authenticated gpt-4 request succeeds and identifies R1 and R2. GTK build
  passes; 13 transport tests pass, including private/actionable context errors.
- Provider error messages are not echoed wholesale. Recognized structured error
  codes yield actionable context/model/quota diagnostics without exposing keys
  or request data. Other errors retain the generic HTTP status.

Shared GTK GSettings startup fix (2026-10-21):

- Menu dialogs could abort with "No GSettings schemas are installed" even
  though Homebrew's merged gschemas.compiled exists. Configure now discovers
  the Homebrew prefix and records its schema directory. pcb-runtime.h applies
  that existing directory before gtk_init, respecting an explicit
  GSETTINGS_SCHEMA_DIR. This covers GTK dialogs globally, not only AI Assist.
- Full GTK build passes. A schema lookup succeeds with XDG_DATA_DIRS deliberately
  pointing to an empty location and GSETTINGS_SCHEMA_DIR initially unset. The
  local asynchronous chat/pending-close fixture passes under the same missing
  default-data setup. No desktop settings are modified.
- If the detected schema installation is removed, the fallback does not invent
  files; reinstall the dependency or supply a valid schema path. Other unrelated
  menu crashes still need individual diagnosis.

PCB AI Assist copied from gEDA-ai (2026-10-21):

- Copy the GTK chat, provider controls, JSON encoder and standard-library Python
  transport, retaining licensing. Adapt context to PCB layout and separate
  pcb-ai preference storage. Expose Window -> AI Assist and AiAssist().
- Native writer now accepts notification control; a read-only stream wrapper
  preserves coordinate formatting policy. The board adapter captures unsaved
  state through a private tmpfile, without SavePCB filename/dirty-state effects.
- New GTK modules pass existing strict flags, including declaration placement.
  Eleven copied transport tests pass. A local GTK fixture verifies two sends
  capture changing board context, asynchronous replies, and closing while a
  helper is pending. No external API calls or real credentials used in tests.
- Credential discovery also searches GEDADATADIR/GEDADATA and HOME .env files,
  after explicit key/environment and selected/launch-directory files. Tests
  isolate HOME and working directory and use dummy keys only. Twelve transport
  tests pass.
- Installed helper lives under share/pcb/ai; source-tree builds fall back to
  their copied helper. Python3 is located by configure. GUI interaction and
  real-provider testing remain for the maintainer.

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
