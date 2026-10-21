#!/bin/sh
#
# Regenerate the Material Symbols icons under 16x16/actions/.
#
# The gEDA action icons (insert-*, select, clone, ...) come from
# gEDA's own artwork in ../../data/.  A handful of gEDA actions have
# no artwork of their own (the gtk-* stock ids in ../../src/actions.c);
# those are taken from Google's Material Symbols set, which is flat,
# single-colour, 24px grid, outlined and Apache-2.0 licensed.
#
# Usage:  ./fetch-material-icons.sh
#
# Requires: curl, rsvg-convert (librsvg).
#
# Only the missing icons are fetched; anything that already exists in
# this directory is left alone.  The result is committed, so a normal
# build never needs network access or rsvg-convert.

set -e

BASE=https://raw.githubusercontent.com/google/material-design-icons/master/symbols/web
OUT=$(dirname "$0")/16x16/actions

# gschem icon id                  Material Symbols name
fetch_one() {
    id=$1
    name=$2

    if [ -f "${OUT}/${id}.png" ]; then
        echo "  ${id}.png exists, skipping"
        return 0
    fi

    svg=$(mktemp)
    url="${BASE}/${name}/materialsymbolsoutlined/${name}_24px.svg"
    if ! curl -sSfL -o "${svg}" "${url}"; then
        echo "error: cannot fetch ${url}" >&2
        rm -f "${svg}"
        return 1
    fi

    rsvg-convert -w 16 -h 16 --background-color=none "${svg}" \
        -o "${OUT}/${id}.png"
    rm -f "${svg}"
    echo "  ${id}.png <- material-symbols/${name}"
}

while read -r id name; do
    [ -n "${id}" ] || continue
    case "${id}" in \#*) continue ;; esac
    fetch_one "${id}" "${name}" || exit 1
done <<'EOF'
gtk-about                       info
gtk-cancel                      cancel
gtk-close                       close
gtk-cut                         content_cut
gtk-enable-accels               keyboard
gtk-execute                     play_arrow
gtk-find                        search
gtk-go-back                     arrow_back
gtk-go-down                     keyboard_arrow_down
gtk-go-forward                  arrow_forward
gtk-go-up                       arrow_upward
gtk-goto-bottom                 vertical_align_bottom
gtk-ok                          check
gtk-paste                       content_paste
gtk-preferences                 settings
gtk-print                       print
gtk-properties                  tune
gtk-quit                        logout
gtk-refresh                     refresh
gtk-revert-to-saved             restart_alt
gtk-save-as                     save_as
gtk-select-all                  select_all
gtk-zoom-fit                    zoom_out_map
gtk-zoom-in                     zoom_in
gtk-zoom-out                    zoom_out
EOF

echo "done"
