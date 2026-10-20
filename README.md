# pcb-ai

An Apple Silicon modernization effort for the original **gEDA PCB** board-layout
editor, maintained by **John Ryan, Aquana Press (Aqnana.net)**.

Companion schematic project: [gEDA-ai](https://github.com/tambotitree/geda-ai).
Upstream PCB: [gEDA PCB developers](https://github.com/gEDA-pcb-developers/pcb).

## Why this project exists

A schematic is only the beginning. We want an engineer to draw a circuit in
gEDA-ai, place its components on a board, route the copper, and print artwork
that can actually be transferred to copper-clad material and etched.

Our first acceptance test is deliberately small:

1. Draw two resistors and connect them in gschem.
2. Convert the schematic using gsch2pcb with physically verified footprints.
3. Open the board, place the components, and route one copper layer.
4. Export a PDF at **exactly 1:1 scale**, with deliberate layer, mirroring and
   polarity settings for laser-printer toner transfer.
5. Check a known dimension on paper and pad spacing against the real parts.

Copper artwork must exclude silkscreen and other unwanted annotations. The
conversion-only TEST_RESISTOR fixture in gEDA-ai is not a manufacturing footprint.
Gerber/drill and milling workflows can follow once this path is verified.

## Current status

**The batch/PostScript build succeeds; the GTK3 editor and a working release
remain unverified.**
Apple Silicon macOS is the initial target. Other platforms are not verified.

- The upstream Git history is preserved, starting from revision
  `a38ac3e999f388219adadecee0add6b56eb36c6e`.
- The untouched upstream `autogen.sh` was tried. `autopoint` completed, but
  bootstrap stopped because `intltoolize` was not installed. The adapted
  English-only `bootstrap.sh` now completes; an isolated batch/PostScript
  configuration and metadata generation also pass. The batch/PostScript
  executable builds and reports the expected version.
- A two-resistor smoke test passes batch load/save/reopen, connection checking
  (including a disconnected negative control), and PostScript/PDF export.
  It uses test footprints; physical manufacture and printer scale are unverified.
- Upstream GUI configuration still requests GTK2 and gtkglext. The GTK3 port
  has not yet been performed.
- Upstream already contains PostScript/EPS, Gerber, and experimental G-code
  export backends. Those facilities will be verified rather than assumed to
  produce correct manufacturing output on the target platform.

The existing [README](README), [README.git](README.git) and [INSTALL](INSTALL)
describe upstream PCB; their dependency and installation instructions are
historical and do not establish a working pcb-ai installation.

## Modernization approach

Work proceeds in consultation with the maintainer, one reviewable step at a time:

- Establish the baseline failure before changing the code.
- Review applicable mechanical migration tools before fixing individual errors.
- Adapt gEDA-ai's bootstrap approach to PCB's own build system and m4 macros.
- Start with an English-only build, preserving translation sources for later.
- Target GTK3 and the existing modern Homebrew toolchain. Use Guile 3 and
  Python 3 where dependencies actually require them; do not add unused runtimes.
- Preserve explicit callback types and enable strict compiler diagnostics.
- Bundle icons instead of relying on a desktop theme installation.
- Keep debugging symbols and test open/edit/save/reopen and physical export scale.

Track stage completion, decisions and test notes in the
[upgrade tracker](UPDATE_PROJECT_TODO.md). See [the project plan](PCB_AI_PLAN.md)
for the original scope and acceptance criteria.
A successful compiler run alone will not count as a working board-production tool.

## Getting the source

```sh
git clone https://github.com/tambotitree/pcb-ai.git
cd pcb-ai
git remote add upstream https://github.com/gEDA-pcb-developers/pcb.git
```

Generate the English-only build system with `./bootstrap.sh` (the historical
`./autogen.sh` delegates to it). It requires Autoconf 2.73, Automake and their
normal macro dependencies, but does not run autopoint or intltoolize. Translation
catalogs remain in `po/`. Configure requires GNU Bison 3.0 or newer.
On macOS, `m4/pcb-macos.m4` prefers Homebrew's keg-only Bison, obtained with
`brew install bison`, without changing your shell PATH. An explicit `YACC`
override is honored but must pass the version check. Native-language support is deferred; `--enable-nls`
is rejected and `--disable-nls` is accepted.

Reproducible Apple Silicon build commands will be documented when verified.
There is no supported binary package from this project yet.

## Release identity

The authoritative version is the literal `AC_INIT` version in `configure.ac`.
The current development checkpoint is `0.1.1-20261020`. The batch/PostScript
smoke test passes; the GUI and manufacturing workflow remain unverified.
Versions follow `MAJOR.MINOR.PATCH-YYYYMMDD`, matching gEDA-ai's release format.

```sh
python3 tools/version.py show
python3 tools/version.py set 0.1.2-20261021  # example next version
python3 tools/version.py check
```

After changing the version, regenerate the build system and rebuild. Autoconf
and Automake use it for `VERSION`, `PACKAGE_VERSION` and `make dist` archive names.
The source version remains available in tarballs without Git or Python at build time.
The Python helper is a maintainer tool.

Development checkpoints may be tagged before the build works. The initial
`0.1.0-20261020` tag records the version tooling; its build and release tarball
are unverified. A tag does not establish a tested release.

For a tested release, pass the build and acceptance tests, commit the sources,
and create an annotated tag with the exact version printed by the helper
(no `v` prefix). After `make dist`, validate both identities before publishing:

```sh
python3 tools/version.py check --tag --archive pcb-ai-0.1.1-20261020.tar.gz
```

`--tag` requires a clean checkout and the matching annotated tag at HEAD.
`--archive` checks the filename, top directory and embedded source version.
The helper never creates tags or pushes. Archive checks do not replace build,
installation or physical print-scale tests. Run its regression tests with
`python3 tools/test_version.py`.

## Support and contributions

- [Issues and development discussion](https://github.com/tambotitree/pcb-ai/issues)
- [Pull requests](https://github.com/tambotitree/pcb-ai/pulls)

Current maintenance belongs to John Ryan / Aquana Press. Please use this
project's tracker for pcb-ai support rather than contacting historical authors.

## License and credits

This project preserves the original PCB authors' history, copyrights and
licenses. PCB is GNU GPL software; see [COPYING](COPYING) and individual source
headers for applicable terms. The upstream [README](README) retains the original
project information. The pcb-ai name does not imply endorsement by the upstream
maintainers or the GNU Project.
