# Upgrade inventory and working data

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
