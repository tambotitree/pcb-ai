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
