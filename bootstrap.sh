#!/bin/sh
# Generate PCB-ai's English-only build system. Translation inputs are retained.
# Run each generator explicitly so failures identify the current stage.
set -eu

cd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

for tool in aclocal autoheader autoconf automake; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "Missing bootstrap tool: $tool" >&2
    exit 1
  fi
done

# PCB uses static archives, not Libtool. There is no libtoolize step or
# gEDA-specific bitmap/Xorn generation. Do not run autopoint or intltoolize.
echo "Running aclocal..."
aclocal -I m4 --force
echo "Running autoheader..."
autoheader --force
echo "Running autoconf..."
autoconf --force
echo "Running automake..."
automake --add-missing --copy --foreign -Wall
echo "Bootstrap complete. Run ./configure with the desired build options."
