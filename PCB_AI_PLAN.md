# pcb-ai companion modernization

## Provenance and scope

- Upstream: https://github.com/gEDA-pcb-developers/pcb.git
- Initial upstream revision: a38ac3e999f388219adadecee0add6b56eb36c6e
- Initial modernization branch: modernization/apple-silicon
- Companion repository: https://github.com/tambotitree/pcb-ai
- Target platform for initial verification: Apple Silicon macOS.
- Proposed companion maintainer: John Ryan, Aquana Press (Aqnana.net).

Retain upstream history, licenses and original author credits. This directory
is a separate Git checkout, ignored by the parent gEDA-ai repository. The companion GitHub repository publishes project status and preserved upstream
history. No working release or new release tag is claimed. Keep origin for the
companion and upstream for the original PCB developers.

## First acceptance test

1. Read the connected two-resistor board produced by gEDA-ai gsch2pcb.
2. Assign physically verified resistor footprints. The TEST_RESISTOR fixture
   is for conversion regression only and must not be used for manufacturing.
3. Place the footprints, route one copper layer, and check connectivity and
   clearance against the chosen fabrication process.
4. Export solid black copper at 1:1 physical scale, with explicit layer,
   mirroring and polarity choices for toner transfer. Exclude silkscreen and
   annotations from the transfer artwork.
5. Convert PostScript to PDF without scaling, or use a verified native PDF
   exporter if available. Include a separate dimensional verification page.
6. Measure the PDF geometry and the laser-printed scale check at Actual Size.
   Verify the printed pads against the actual component before etching.

## Incremental work

- [x] Bootstrap the untouched upstream tree: autopoint passed; intltoolize missing.
- [ ] Establish reproducible version/data-directory handling for source trees
  and tarballs. Keep PCB data separate from gEDA-ai runtime data.
- [ ] Build the core and PostScript exporter without a GUI first; preserve
  debug symbols. Add real board-to-artwork regression checks.
- [ ] Enable strict source diagnostics outside configure feature probes:
  -Wall -Wextra -Werror -Wpedantic -Wstrict-prototypes
  -Wmissing-prototypes -Wformat=2. Keep callback types explicit.
- [ ] Assess the GTK2/gtkglext GUI dependency and plan a native macOS-compatible
  modernization. Do not change dependency version checks without porting APIs.
- [ ] Bundle application/action icons; reuse suitable gEDA-ai artwork with its
  attribution. Verify GUI operation without a desktop icon theme.
- [ ] Verify open/edit/save/reopen and a routed-board print/export workflow.
- [x] Start the public companion repository with an honest status README.
- [ ] Publish a working release only after the baseline can be reproduced and
  its limitations are documented.

Upstream INSTALL already describes PostScript/EPS, Gerber and experimental
G-code export backends. Prioritize PostScript-to-PDF and physical scale before
milling features. Reuse verified behavior rather than writing another router
or graphics exporter.
