## -*- makefile -*-
##
## Top level automake file for PCB

DIRS=		w32 win32 gts src data lib newlib example tools tutorial README_FILES tests
SUBDIRS=	${DIRS} @DOC@
DIST_SUBDIRS=	${DIRS} doc

EXTRA_DIST=	config.rpath globalconst.h README.git README.w32 README.win32 icon-theme-installer

DISTCLEANFILES= configure.lineno src/gpcb-menu.res.tmp src/pcb-menu.res.tmp


ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS := ${DISTCHECK_CONFIGURE_FLAGS} --disable-update-mime-database --disable-update-desktop-database GTK_UPDATE_ICON_THEME_BIN=true --with-gui=batch


# Release tooling must also work in distributed sources without Git.
EXTRA_DIST += README.md PCB_AI_PLAN.md UPDATE_PROJECT_TODO.md tools/version.py tools/test_version.py

EXTRA_DIST += upgrade_data/README.md tools/upgrade_inventory.py

EXTRA_DIST += tools/conversion.py tools/python_conversion.py tools/autoconf_conversion.py

# Preserve translation inputs without requiring translation tools to build.
EXTRA_DIST += bootstrap.sh autogen.sh po/LINGUAS po/POTFILES.in po/POTFILES.skip \
  po/de.po po/fr.po po/nl.po po/pt_BR.po po/ru.po

EXTRA_DIST += tools/c_conversion.py tools/scheme_conversion.py

EXTRA_DIST += tools/c_signature_conversion.py

EXTRA_DIST += m4/pcb-macos.m4

EXTRA_DIST += GUI_REFACTOR_TODO.md

EXTRA_DIST += tools/gtk3_build_conversion.py

EXTRA_DIST += tools/struct_macro_conversion.py

EXTRA_DIST += tools/gtk_api_conversion.py tools/test_gtk_api_conversion.py

EXTRA_DIST += tools/timestamp_conversion.py

EXTRA_DIST += upgrade_data/gtk_api_migrations.json

EXTRA_DIST += tools/visibility_renderer_conversion.py

EXTRA_DIST += tools/test_visibility_renderer.py tools/test_visibility_renderer.c

EXTRA_DIST += tools/drawing_conversion.py tools/drawing_backend_rules.py tools/drawing_ui_rules.py
EXTRA_DIST += tools/menu_conversion.py
EXTRA_DIST += tools/gtype_conversion.py
EXTRA_DIST += tools/menu_actions_conversion.py
EXTRA_DIST += tools/selector_actions_conversion.py
EXTRA_DIST += tools/dialog_layout_conversion.py
EXTRA_DIST += tools/callback_conversion.py upgrade_data/callback_parameters.json
EXTRA_DIST += tools/pointer_conversion.py
EXTRA_DIST += tools/hid_initializer_conversion.py
EXTRA_DIST += tools/hid_bounds_conversion.py
EXTRA_DIST += tools/hid_flags_conversion.py
EXTRA_DIST += tools/benchmark_conversion.py
EXTRA_DIST += tools/combo_conversion.py
EXTRA_DIST += tools/desktop_hints_conversion.py
EXTRA_DIST += tools/array_loop_conversion.py
EXTRA_DIST += tools/dialog_stock_conversion.py
EXTRA_DIST += tools/reverse_loop_conversion.py
EXTRA_DIST += tools/layer_loop_conversion.py
EXTRA_DIST += tools/render_cache_conversion.py tools/sentinel_audit.py
EXTRA_DIST += upgrade_data/sentinel_candidates.json

EXTRA_DIST += tools/topbar_appearance_conversion.py

EXTRA_DIST += tools/xpm_stream_conversion.py

EXTRA_DIST += screenshots/two_resistors.jpg
