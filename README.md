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

**Project started; no working pcb-ai build or release is claimed yet.**
Apple Silicon macOS is the initial target. Other platforms are not verified.

- The upstream Git history is preserved, starting from revision
  `a38ac3e999f388219adadecee0add6b56eb36c6e`.
- The untouched upstream `autogen.sh` was tried. `autopoint` completed, but
  bootstrap stopped because `intltoolize` was not installed.
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

See [the project plan](PCB_AI_PLAN.md) for the acceptance criteria and work list.
A successful compiler run alone will not count as a working board-production tool.

## Getting the source

```sh
git clone https://github.com/tambotitree/pcb-ai.git
cd pcb-ai
git remote add upstream https://github.com/gEDA-pcb-developers/pcb.git
```

Reproducible Apple Silicon build commands will be documented when verified.
There is no supported binary package from this project yet.

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
