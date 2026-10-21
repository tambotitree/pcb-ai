# Upgrade inventory and working data

For each build stop, search the whole PCB-ai source tree for the same migration
pattern and capture the matches. Extend a Python converter (or add a dedicated
one), preview its diff, apply it to all confirmed instances, and rebuild. Record
ambiguous matches for review; similar names alone do not establish compatible
callback signatures. See `../UPDATE_PROJECT_TODO.md` for the agreed cycle.

`source_inventory.json` is the versioned inventory and per-file review record.
Regenerate it from the repository root with:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/upgrade_inventory.py
```

The scanner reads tracked text files from the current working tree and recognizes
C/C++ headers and sources, Lex/Yacc, Python, Scheme, shell, Perl, Tcl, M4,
Autoconf/Automake, menu resources and GTK UI files. It also recognizes script
shebangs and `.in` templates. It skips binaries, symlinks and this data directory.
Add new source files to Git's index before regenerating so they are included.
Documentation, board files and footprints are outside this source-code inventory.

Each entry records its path, language, coarse area, line count, SHA-256 and
line-numbered migration clues. Clues are regex matches, including matches in
comments; they are prompts for inspection, not confirmed defects or complete
dependency analysis. Files with no clues still require review. No converter,
compiler, application or network request is run by the scanner.

Edit a file's `review` object to track human progress:

```json
{
  "status": "in_progress",
  "reviewed_sha256": null,
  "notes": "Inspect event coordinate conversions before changing drawing APIs.",
  "checks": []
}
```

Suggested statuses: `unreviewed`, `in_progress`, `blocked`, `verified`,
`not_applicable`, `needs_review`. Add dated check results and commands to `checks`.
When a review is complete, copy the entry's `sha256` into `reviewed_sha256`.
The scanner preserves review fields; if that hash no longer matches the source,
it sets `needs_review`. Deleted entries are retained under `removed_files`.
An unchanged tree produces unchanged JSON. Schema version is currently 2.

Use `scratch/` for temporary JSON responses, diffs, logs and experiments.
Its contents are ignored by Git. Keep credentials and secrets out of inventory
notes. Record stage-level decisions in `../UPDATE_PROJECT_TODO.md`.

The inventory describes the current working tree, not necessarily the baseline
tag. The baseline tag is a reference point; per-file hashes identify scanned
content. Regenerate after committing new source files or changing them.

## Separate conversion passes

Use one converter for each file type. Both select files from the JSON inventory:

```sh
python3 tools/python_conversion.py
python3 tools/autoconf_conversion.py
```

These commands only preview diffs and write JSON reports under `scratch/`.
To apply a reviewed pass, add `--apply`; use repeatable `--path PATH` options
for narrower selection. `--tool PATH` selects a specific installed converter.
Regenerate the inventory after editing files, before another pass. A stale file
hash blocks conversion. If any selected conversion fails, no candidates from
that pass are applied. Applied candidates have original-byte backups in the
report directory. A mechanical conversion is not a completed build or review.

`python_conversion.py` runs `2to3-3.12` in temporary storage, updates legacy
Python shebangs and checks candidate syntax with Python 3. Files already declaring
Python 3 are syntax-checked and left alone. This does not validate runtime APIs.
`autoconf_conversion.py` runs `autoupdate` in temporary storage with copied local
macro context. Review obsolete-macro warnings and generated edits before applying.
`conversion.py` shares selection, staging checks, diffs, backups and reporting;
file-type transformation logic lives in the separate scripts. `c_conversion.py` handles a narrow GTK constructor pass in `src/hid/gtk/`:
nonhomogeneous boxes, panes and separators. It does not port drawing/events or
switch the configured GTK dependency. `scheme_conversion.py` checks forms using
the Guile 3 reader without evaluating gEDA procedures; no rewrite rule is currently
selected for the two Scheme scripts.

## Version records

The inventory records `project_version` from `configure.ac`. Each file has
`versions.first_seen_project_version`, `versions.scanned_project_version` and a
`versions.conversions` history. An applied conversion appends its project version,
converter executable and version output, executable hash, UTC timestamp and
before/after content hashes. Its initial status is `applied_unverified`; a source
rewrite does not prove successful compilation or runtime behavior. For 2to3,
the version output identifies the installed Python 3.12 runtime; the executable
path and hash identify the selected converter. Preview reports do not append
completed conversions. Regeneration preserves conversion history.

## C signature conversion

`python3 tools/c_signature_conversion.py --path gts/predicates.c` previews
simple K&R pointer-parameter definitions as explicitly typed definitions.
Candidate declarations are included in the scratch JSON report diagnostics.
Add `--apply` to write reviewed changes and record conversion history. Existing
headers are not rewritten: compare them first, then compile the candidate source
against its included header. Scalar promotion rules, complex declarators and
missing types are outside this converter's rules. Function bodies are untouched.

`gtk3_build_conversion.py` selects the experimental GTK3 configure dependency
and rejects the legacy gtkglext path. It does not convert renderer types or
claim a working GUI. It uses the same preview/apply and history workflow.

`struct_macro_conversion.py --path src/global.h` normalizes the redundant
ANYOBJECTFIELDS macro terminator. It leaves callers and field layout intact;
the first application was checked with compiled size/offset comparisons.

`gtk_api_conversion.py` holds a RULES table of file scopes, patterns, replacements
and rationales. A `None` replacement reports `needs_rule` without rewriting.
Preview selected files with repeatable `--path` arguments; add `--apply` after
review. Run `python3 tools/test_gtk_api_conversion.py` for the rule tests.

## One-time migration policy

Conversion scripts record historical migration steps. Apply a reviewed rule once,
then maintain the converted production source directly. They are not run by
bootstrap, configure or make and must not regenerate code after manual changes.
Idempotence checks validate a rule before use, not a recurring build process.
Windows-specific portability changes are deferred until the Windows port.

`timestamp_conversion.py` records the completed migration from GTimeVal to
struct timespec, stat member detection and coordinated timestamp comparisons,
following gschem. The converted source is now authoritative.

`gtk_api_migrations.json` is the human-reviewed rule table read by the GTK API
script. It stores compiler hints separately from selected replacements and links
to actual companion gschem implementations. Report-only rows retain unresolved
ownership/type changes until a coordinated conversion rule is ready. These
references are evidence, not a dependency on gschem at normal build/runtime.

`visibility_renderer_conversion.py` records the completed coordinated visibility
cell-renderer migration (guarded against an unexpected original hash). Maintain
the production source directly. `test_visibility_renderer.py` compiles and tests
that actual source against GTK3 independently of the full PCB window. It requires
a configured experimental GTK3 build and returns skip status 77 without a display.
