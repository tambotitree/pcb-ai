# PCB-ai upgrade tracker

Work through these stages in consultation with John Ryan. Review each stage's
results before changing direction. Record commands, failures and decisions here
as we go; a planned check is not a completed check.

## Starting point — 2026-10-20

- [x] Preserve original PCB history and configure the upstream remote.
- [x] Create the public companion repository: https://github.com/tambotitree/pcb-ai.
- [x] Publish the project README and initial plan.
- [x] Establish explicit version handling and publish annotated development tag
  `0.1.0-20261020` at commit `b3ad597e`.
- [x] Try untouched upstream `autogen.sh`: autopoint completed, then bootstrap
  stopped because `intltoolize` was missing.
- [x] Inventory installed migration tools: Homebrew `autoupdate`,
  `2to3-3.12`, `gtk-builder-tool` and `glibtoolize` are available.
- [x] Inspect gEDA-ai's Python refactoring helpers. They generate and validate
  gschem action registries; no general GTK2-to-GTK3 converter was found.

The development tag is not a verified working release. No successful PCB-ai
build, conventional release tarball or physical artwork test is claimed yet.
The adapted `bootstrap.sh` has not been added.

## Working discipline

- Current priority: obtain a compiled GTK executable, however rough. Defer broad
  cleanup that does not block that build. When touching a directory or function,
  consult `upgrade_data/sentinel_candidates.json` and
  `upgrade_data/signedness_candidates.json`, review relevant entries and update
  their status/notes. These audit lists are follow-up queues, not release gates.

- Counts and indices use appropriate numeric types; named states use enums and
  two-valued conditions use bool. A cache value retains its domain type and uses
  explicit validity instead of an out-of-domain numeric sentinel. Audit -1 uses
  systematically, preserving documented error contracts and real arithmetic.

- Optional desktop integration is outside the application scope. Remove legacy
  window-manager class, pager and taskbar policy hints; retain application-owned
  titles, dialog parents, modality and input/rendering behavior. Use bundled
  assets rather than desktop theme dependencies as icon consumers are migrated.

### Build-stop conversion cycle (agreed 2026-10-20)

The next coordinated work set is documented in
[`upgrade_data/menu_migration_scope.md`](upgrade_data/menu_migration_scope.md),
including C sources, headers, callers and menu resource definitions.

1. Capture the compiler failure and identify the underlying migration pattern.
2. Search the entire PCB-ai source tree with `rg`, listing all matching cases.
3. Add a reviewed rule to an existing Python converter, or create a dedicated
   converter when the pattern needs coordinated declarations and bodies.
4. Preview the complete diff, then apply the conversion to every confirmed
   instance across the project. Flag ambiguous cases instead of guessing;
   callback parameters and return types must match the actual signal contract.
5. Rebuild, record the compiled artifacts and next blocker, and repeat.

Compilation is the immediate acceptance target. Avoid unrelated UI redesigns
or rendering experiments during these cycles. Converters remain one-time
migration tools, never bootstrap or normal build dependencies. A temporary
warning exemption is recorded debt, not a completed API conversion.

- Initial verified platform: Apple Silicon macOS.
- Preserve upstream history, licenses and author credits.
- Target GTK3 and modern installed tools. Use Python 3 or Guile 3 only where
  the project actually needs them; do not introduce unused dependencies.
- Start with an English-only build; preserve translation sources for later.
- Keep pointer and callback types explicit. Cast only at required API boundaries.
- Use debug symbols without stripping. Enable strict application diagnostics:
  `-Wall -Wextra -Werror -Wpedantic -Wstrict-prototypes
  -Wmissing-prototypes -Wformat=2`. Keep configure feature probes separate.
- Make small, reviewable changes. Record validation and known limitations.
- Keep existing tags fixed. Use a new version for the next checkpoint.

## 1. Preserve the baseline

- [x] Keep the initial annotated development tag as the recovery point.
- [ ] Record the next checkpoint version when it is chosen.

Notes: initial upstream revision is
`a38ac3e999f388219adadecee0add6b56eb36c6e`.

## 2. Inventory migration scope — in progress

- [x] Generate a repeatable source/build inventory with file hashes, migration clues
  and review metadata in `upgrade_data/source_inventory.json`.
- [ ] Map GTK2 drawing and backing-buffer code, coordinate transforms and redraws.
- [ ] Map pointer events, snapping, selection and routing interactions.
- [ ] Map widgets, menus, dialogs, icons and ownership/lifetime handling.
- [ ] Identify gtkglext's role and replacement options.
- [ ] Inventory Python helpers and their runtime requirements.
- [ ] Separate mechanical conversions from manual changes; record proposed order.

Notes / decisions:

## 3. Review mechanical conversions

- [x] Run `autoupdate`, inspect its diff and retain only understood changes.
- [ ] Preview Python changes with Homebrew `2to3-3.12` before applying them.
- [ ] Assess whether narrow, project-specific conversion scripts would help.
- [ ] Check each conversion independently for preserved types and behavior.

Notes / commands / results:

## 4. Adapt bootstrap

- [x] Adapt gEDA-ai's `bootstrap.sh` to PCB's actual build requirements.
- [x] Implement the English-only baseline, retaining translation sources.
- [x] Remove or replace obsolete internationalization build steps consistently.
- [x] Add required bootstrap script and distribution rules to the working tree;
  commit remains pending. No new project-specific m4 macro is needed.
- [x] Run bootstrap and clear failures one at a time.
- [ ] Commit a reproducible bootstrap stage.

Notes / commands / results:

## 5. Configure the initial build

- [ ] Verify configured version and package identity.
- [ ] Establish PCB-specific installation/data paths, separate from gEDA-ai.
- [ ] Configure the batch interface and PostScript exporter first.
- [ ] Verify debug flags and dependency selection.

Notes / commands / results:

## 6. Build core and exporters

- [ ] Build with strict application diagnostics; fix errors in small groups.
- [ ] Read a board, write it and reload it.
- [ ] Export PostScript and inspect output.
- [ ] Add meaningful regression checks for verified behavior.

Notes / commands / results:

## 7. Port the GUI to GTK3

- [ ] Agree on the gtkglext replacement after inspecting its role.
- [ ] Port window creation and widget ownership.
- [ ] Port menus and dialogs.
- [ ] Port drawing, preserving transforms and redraw behavior.
- [ ] Port pointer events, snapping, selection and routing interactions.
- [ ] Verify each step before proceeding.

Notes / decisions / results:

## 8. Bundle icons and verify editor operation

- [ ] Bundle application/action icons with appropriate attribution.
- [ ] Verify operation without desktop-theme icon dependencies.
- [ ] Open the editor with a usable canvas.
- [ ] Open, edit, save and reopen a board.

Notes / commands / results:

## 9. Physical acceptance test

- [ ] Import the two-resistor connected schematic through gsch2pcb.
- [ ] Assign physically verified resistor footprints. The gEDA-ai
  `TEST_RESISTOR` fixture is for conversion tests, not manufacturing.
- [ ] Place components and route one copper layer.
- [ ] Check connectivity and clearance for the intended etching process.
- [ ] Export solid black copper to PDF at exactly 1:1 physical scale.
- [ ] Verify layer selection, mirroring and polarity; exclude silkscreen and
  annotations from toner-transfer artwork.
- [ ] Measure PDF geometry and a laser print made at Actual Size.
- [ ] Compare printed pad spacing with real components before etching.

Notes / measurements / results:

## 10. Reproducible release

- [ ] Choose and set the next version using `tools/version.py`.
- [ ] Generate the conventional release tarball with `make dist`.
- [ ] Unpack outside the checkout and build/test without its Git metadata.
- [ ] Commit release sources and create the matching annotated tag.
- [ ] Validate tag and archive with `tools/version.py check --tag --archive PATH`.
- [ ] Document tested platform, commands and remaining limitations; publish.

Notes / commands / results:

## Deferred work

- Translation build restoration.
- Platforms beyond Apple Silicon macOS.
- Milling/G-code acceptance tests after the toner-transfer PDF path works.

## Progress log

### 2026-10-20

Agreed on the staged upgrade plan. Next action: inventory the migration scope
before applying further conversions or build-system changes.

Initial automated inventory: 369 source/build inputs, including 36 GTK GUI
files. Regex clues identify 2 files mentioning gtkglext and 7 with legacy drawing
patterns. These are inspection leads; manual mapping and dependency review remain.
Review-note preservation, changed-content detection and deterministic regeneration
were checked with local fixtures. No conversion or build was run in this stage.

### Rough inventory scope and conversion scripts

The maintainer chose a rough type inventory rather than detailed manual mapping
at this stage. Defer the unchecked detailed mapping items in stage 2 until a
specific migration needs them. Separate inventory-driven Python and Autoconf
conversion scripts now preview diffs and save scratch reports. No source
conversion has been applied yet; conversion results still need build checks.

### 2026-10-20 — Autoconf conversion

Applied `tools/autoconf_conversion.py --apply` with Homebrew Autoconf 2.73.
Reviewed generated changes: `AC_USE_SYSTEM_EXTENSIONS`, `AS_HELP_STRING`, modern
compiler selection and standard-header checks. Removed generated warning stubs
and duplicate compiler requirements; retained standard-header checks because
`src/getline.c` uses `STDC_HEADERS`. The project version remains `0.1.0-20261020`.

In a temporary copy, `aclocal -I m4` and `autoconf` both exited 0. Remaining
warnings concern bundled gettext/m4 macros and Lex configuration. This is a
script-generation check, not a successful configure/build. Full bootstrap still
requires the agreed English-only replacement of the intltool path. Next: adapt
bootstrap and its matching configuration/data-generation rules.

### 2026-10-20 — English-only bootstrap

Added `bootstrap.sh`; `autogen.sh` now delegates to it. PCB has no Libtool/Xorn
setup, so those gEDA-specific steps were omitted. Removed gettext/intltool build
requirements and translation-only menu extraction. Runtime menu resources remain.
Preserved all `.po` catalogs and explicitly included translation inputs in dist.
English desktop/XML generation replaces intltool merging. Added `AM_PROG_AR`.
Icon installation now uses Automake's install-sh rather than autopoint's
mkinstalldirs.

Validation: bootstrap passed in the checkout and a temporary copy containing
only tracked source inputs plus the new bootstrap script. Thus old untracked
intl macro outputs are not required. In that clean copy, batch/PostScript
configuration with debug flags, disabled docs/GL/DBus/NLS succeeded. Metadata
make targets passed, XML parsed and `ENABLE_NLS` remained undefined.

Logs: `upgrade_data/scratch/bootstrap.log`, `clean-bootstrap.log` and
`configure-smoke.log`. Remaining warnings: obsolete bundled m4/Lex constructs,
GNU-make extensions and legacy documentation rules. Compilation and full
installation/data-path validation are not yet done. No commit or push performed.

### 2026-10-20 — Python and GTK constructor conversion passes

Ran `tools/python_conversion.py --apply` and `tools/c_conversion.py --apply`.
The completed transformations are the PCB Python helper's Python 3 shebang and
72 GTK constructor replacements in 13 GUI files. Repeating both passes produced
no further edits. All 9 inventoried Python files parsed under Python 3, and the
PCB helper constructed and serialized a basic board fixture. The 72 constructor
calls compiled against installed GTK3 headers with strict diagnostics in a small
signature-check translation unit. This is not a full GUI compilation or runtime
check. Drawing, event and gtkglext migration remain pending.

The two Scheme files were read successfully by Guile 3 without rewrites;
application-specific procedure compatibility remains unverified. Generic Guile
migration tools were investigated; no general automatic Scheme upgrader was
found. The Scheme script currently performs a reader check, not a conversion.

### 2026-10-20 — First rough build

Bootstrap and out-of-source configure passed. Ran `make -j4` for batch GUI and
PostScript exporter, debug enabled, `CFLAGS=-g3 -O0`, `CXXFLAGS=-g3 -O0`, no
`-Werror`. Compiler warnings remained nonfatal. Build directory:
`upgrade_data/scratch/build-batch`; full commands and output are in
`configure-output.log` and `build-output.log` there.

First blocker: bundled `gts/predicates.c` has four K&R-style function definitions
(`orient2d`, `orient3d`, `incircle`, `insphere`). Configure selected a compiler
invocation using `-std=gnu23`, which rejects that syntax. The build stops at
`predicates.o` before reaching the PCB core. Proposed next change: explicit typed
parameter lists preserving the numerical implementation. GTK compilation and
runtime are not yet tested. No source fix was made during this baseline attempt.

### 2026-10-20 — Signature converter preview

Added separate `tools/c_signature_conversion.py` for simple old-style
pointer-parameter definitions. `gts/predicates.h` already has the correct typed
declarations, and predicates.c includes it. The preview produces four typed
signature replacements without touching numerical bodies. A staged candidate
compiled with `cc -std=gnu23 -fsyntax-only` against the existing header and batch
configuration. Idempotence and exclusion of scalar parameter patterns passed.
The actual predicates.c file is not yet modified; next apply the reviewed pass
and resume the rough build.

### 2026-10-20 — Applied signatures; parser generator blocker

Applied `tools/c_signature_conversion.py --path gts/predicates.c --apply`.
Resumed `make -j4`; bundled GTS compiled and its static archive was produced.
Numerical function bodies were left intact. The next failure is parser generation:
`src/res_parse.y:37` rejects `%name-prefix "res"` with Apple's `/usr/bin/bison`
2.3. No Homebrew Bison is installed. Use a compatible newer parser generator
before considering grammar or generated-body changes. Build log:
`upgrade_data/scratch/build-batch/build-after-signatures.log`.

Maintainer preference reaffirmed: reusable conversion tools for repeated patterns
before hand edits. Existing GTK and signature passes are narrow rules, not a
complete C-body modernization. Record new rules and preview them before applying.

### 2026-10-20 — Modern Bison in the Autotools chain; first successful build

Installed Homebrew Bison 3.8.2. Added `m4/pcb-macos.m4` and called its
`PCB_PROG_BISON` macro from configure.ac. On Darwin build machines it prefers
Homebrew's keg-only Bison, unless YACC was explicitly supplied. All configurations
require GNU Bison >= 3.0 and record PCB_BISON_VERSION. The user's shell PATH is
unchanged. The m4 file is included in distribution rules.

Bootstrap and configure recheck passed. Configure selected
`/opt/homebrew/opt/bison/bin/bison -y`; the batch/PostScript `make -j4` completed
with exit 0. The resulting Mach-O arm64 executable reports
`PCB version 0.1.0-20261020`. Debug flags remain `-g3 -O0`; warnings are nonfatal.
An explicit `YACC=/usr/bin/bison -y` test correctly failed with the minimum-version
message. Parser sources did not need a handwritten compatibility rewrite.

Logs: `upgrade_data/scratch/bootstrap-bison.log`, and
`build-batch/configure-modern-bison.log` / `build-modern-bison.log` under scratch.
GUI compilation, functional board/export tests and print-scale acceptance remain
pending. This is a successful batch build, not a tested GUI or manufacturing release.

### 2026-10-20 — Two-resistor load/save/connectivity/export smoke test

Converted gEDA-ai's connected.sch through gsch2pcb: R1, R2 and LINK (R1-2/R2-1)
preserved. Prepared an arranged copy with a top-layer trace as a text fixture;
this does not test interactive placement/routing. The batch executable loaded,
imported the netlist, saved and reopened it. Reopened routed board: zero rats.
Negative control with trace removed: exactly one rat. Resistor values and netlist
survived saving.

PostScript export and Ghostscript PDF conversion passed. Produced a separate
single-page copper-only PDF without alignment marks, outline or legend; top
copper is unmirrored. At 600 dpi, rendered copper bounds match the known test
geometry (0.254 x 11.43 mm). Rendered and visually inspected the final copper.
Artifacts and logs: `upgrade_data/scratch/two-resistor-test/`.
Summary: `upgrade_data/two_resistor_test.json`. PDF:
`output/pdf/two-resistor-copper-test.pdf`.

Remaining findings: batch calls unimplemented PointCursor action; uninstalled
library scripts require the build/lib path. Test footprints remain unsuitable
for manufacturing. Real footprints, GUI routing, mirroring and physical printer
scale still require acceptance checks. This smoke test is a pass, not completion
of the physical acceptance stage.

### 2026-10-20 — Checkpoint 0.1.1-20261020

Advanced the authoritative version to 0.1.1-20261020, rebuilt the batch/PostScript
executable and verified its reported version. Reopened and saved the connected
two-resistor fixture with that executable; zero rats remained. Release-tool
regression tests passed (3/3). Ran `make clean` successfully afterwards; the
executable and object files are removed, while scratch logs and test outputs
remain. Rebuild with make in the existing batch build directory when resuming.

Checkpoint scope: English-only bootstrap, modern Bison selection through m4,
repeatable conversion tools/inventory, typed GTS signatures, initial GTK
constructor conversion, and successful batch/connectivity/PostScript/PDF smoke
checks. GUI migration, physical footprints, toner-transfer mirroring and printer
scale remain unverified. No conventional release tarball has been tested.

### 2026-10-20 — GUI refactor list

Added `GUI_REFACTOR_TODO.md`, based on the actual gschem action, icon,
appearance and rendering repairs. It preserves PCB's existing C action/HID
interfaces and resource menus. Next proposed work is a separate GTK3 build
boundary; the initial rendering-backend choice needs discussion before porting.

Maintainer direction: automate repeated GUI migrations with separate Python
conversion scripts driven by the inventory. Add and validate a rule first;
preview/apply/compile and record hashes/results rather than hand-editing each
occurrence. GUI_REFACTOR_TODO.md now includes the automation passes and gates.

### 2026-10-20 — GUI cycle 1

Experimental GTK3 build configuration established with a dedicated Python
converter; preview, apply, idempotence, bootstrap and configure checks passed.
Compiler now reaches GTK GUI sources and stops at removed GTK2 types in gui.h.
See GUI_REFACTOR_TODO.md for the bounded next pass. No GUI executable yet.

### 2026-10-20 — GTK cycle 2: strict warnings

Scoped -Wall/-Wextra/-Werror/-Wpedantic to libgtk.a. The first automated cleanup
normalized a shared object-field macro terminator; layout-preservation and
idempotence checks passed. Build diagnostics now point at GTK/GLib deprecations,
removed types and unused callback parameters. Details in GUI_REFACTOR_TODO.md.

### 2026-10-20 — GTK cycle 3: rule table

Table-driven GTK API converter added and applied for adjustment fields, dialog
object casts and scrollbar constructors. Four tests pass; GtkObject field error
cleared. Lifetime-sensitive API changes are reported for new coordinated rules.

### 2026-10-20 — GTK cycle 4: timestamps cleared

Followed gschem's timespec/stat approach with a coordinated one-time Python
conversion. Strict build clears GTimeVal; actual converted-function behavior
checks passed. Current platform selects st_mtimespec. Windows debt is deferred.
Maintainer policy: conversions are one-time migrations, not recurring build or
regeneration steps; converted production code becomes the source of truth.

GTK conversion evidence now lives in upgrade_data/gtk_api_migrations.json: both
compiler suggestions and concrete gschem implementations are recorded. Next
coordinated rule can port the visibility cell renderer using GTK-provided Cairo.

### 2026-10-20 — GTK cycle 5: visibility renderer passed

Applied the coordinated one-time renderer conversion. Its production object
builds with strict flags. Standalone GTK checks pass for rendering variants,
clipping/Cairo preservation, color fallback, toggle signal and finalization.
The shared drawing-header blockers remain in the next object. See the GUI list
and API table for the rule, precedent and validation details.

### AI Assist visual/edit follow-up (2026-10-21)

- [x] Fresh viewport PNG and explicit Send image option.
- [x] At most one automatic image continuation per turn, with visible status.
- [x] Selected-pad width proposal, explicit Apply, undo and image redraw.
- [x] Enlarge existing selected plated-via copper rings; drill/position/layers unchanged.
- [x] Shared operation registry and selection manifest (components/pads/vias/holes/traces).
- [ ] Add separate surface pads over vias, with layer/geometry/connectivity contracts.
- [ ] General lookup/connectivity tools and constrained routing/creation operations.
- [ ] Missing-reference dialogue: resolve existing objects or gather placement and part ID/pinout.
- [ ] Add length/area resizing and object highlighting with stable identifiers.
- [ ] Broaden DRC/clearance validation and provider vision-capability discovery.

### Reviewed JSON patches (2026-10-21)

- [x] Exact memory-only unified diff staging, supplied-document whitelist and
  SHA-256 snapshot checks; reject fuzzy context, unknown paths and mixed actions.
- [x] Plan -> Act -> review popup -> Implement for live PCB edits. Review displays
  summary, diff, native parser validation and before/after viewport images.
- [x] Cancel leaves the board untouched; Implement rechecks snapshot/selection,
  updates live geometry/image and provides one whole-board native Undo/Redo.
- [x] Test invalid native PCB rejection and cancellation in the GTK process.
- [ ] Explicitly attach a schematic from its owning editor, validate with the
  native gEDA parser and coordinate undo before enabling `.sch` application.
  Never infer a paired file or silently edit it on disk.
- [ ] Expose optional model sampling controls (including temperature); preserve
  provider defaults for models that do not accept those parameters.
- [ ] Broaden geometry/DRC validation beyond native syntax and operation bounds.

### Release checkpoint 0.1.3-20261021

- User confirmed visible circular-pad edits through AI Assist and requested an
  annotated tag, commit and push. Keep generated objects, scratch candidates,
  logs and credentials out of the commit.
- Includes multi-provider AI Assist, selection context, optional viewport PNG,
  Plan/Act/review/Implement, exact host-generated patches, native Undo/Redo and
  runtime visibility preservation, plus GTK startup/menu lifetime fixes.
- Current validation: 32 offline tests; native GUI regression including actual
  canvas pad pixels; live gpt-4 disposable-fixture proposal and native export.
- Remaining scope limits: explicit schematic integration, model sampling controls,
  layer-count edits and full geometry/DRC validation.

- Release checks: both full GTK and batch rebuilds pass at 0.1.3-20261021;
  32 AI tests, three version tests and the final native GUI regression pass.

### Usable empty-board startup size (2026-10-26)

- [x] Match gschem's two-thirds monitor work-area default and 1280x720 minimum
  where the display permits, preserving larger saved sizes within the monitor.
- [x] Rebuild GTK and test empty startup with fresh preferences and a disposable
  copy of the user's old small preferences. Both start at 1280x720 here.
- [x] Full native GUI regression passes at the new size, including visible pad
  pixels, reviewed patches, native Undo/Redo and local mock-provider flow.

### Release 0.1.4-20261026

- User approved tagging/pushing the matching laptop-sized startup checkpoint.
- Empty boards and old small preferences start at a usable 1280x720 where the
  monitor allows; retain larger saved sizes within the usable screen.
- Full GTK build and empty-board/full native GUI regressions pass.
