# pcb-macos.m4 -- PCB-ai build-tool selection on macOS
# serial 1

dnl PCB_MACOS_BUILD_TOOLS
dnl Prefer Homebrew's keg-only Bison on Darwin build machines. An explicit
dnl YACC is authoritative. Do not modify the user's shell or global PATH.
AC_DEFUN([PCB_MACOS_BUILD_TOOLS], [
  AC_REQUIRE([AC_CANONICAL_BUILD])
  case "$build_os" in
    darwin*)
      AS_IF([test -z "$YACC"], [
        AC_PATH_PROG([PCB_BREW], [brew])
        AS_IF([test -n "$PCB_BREW"], [
          pcb_bison_prefix=`"$PCB_BREW" --prefix bison 2>/dev/null`
          AS_IF([test -x "$pcb_bison_prefix/bin/bison"], [
            YACC="$pcb_bison_prefix/bin/bison -y"
          ])
        ])
      ])
      ;;
  esac
])

dnl PCB_PROG_BISON
dnl Select the parser generator through Autoconf, then require GNU Bison 3+.
dnl YACC includes its command arguments, as specified by AC_PROG_YACC.
AC_DEFUN([PCB_PROG_BISON], [
  PCB_MACOS_BUILD_TOOLS
  AC_PROG_YACC
  AC_MSG_CHECKING([for GNU Bison version 3.0 or newer])
  pcb_bison_banner=`$YACC --version 2>/dev/null | sed -n '1p'`
  case "$pcb_bison_banner" in
    *GNU\ Bison*) ;;
    *) AC_MSG_ERROR([GNU Bison 3.0 or newer is required. Install bison and set YACC.]) ;;
  esac
  PCB_BISON_VERSION=`echo "$pcb_bison_banner" | sed 's/.* //'`
  AS_VERSION_COMPARE([$PCB_BISON_VERSION], [3.0], [
    AC_MSG_ERROR([GNU Bison $PCB_BISON_VERSION is too old; require 3.0 or newer.
On macOS install Homebrew bison, or set YACC to a modern bison command.])
  ])
  AC_MSG_RESULT([$PCB_BISON_VERSION ($YACC)])
  AC_SUBST([PCB_BISON_VERSION])
])

 dnl PCB_GTK_RUNTIME_SCHEMAS
 dnl Standalone Homebrew executables need the merged GTK schema directory.
AC_DEFUN([PCB_GTK_RUNTIME_SCHEMAS], [
  pcb_gtk_schema_dir=""
  case "$build_os" in
    darwin*)
      AC_PATH_PROG([PCB_BREW], [brew])
      AS_IF([test -n "$PCB_BREW"], [
        pcb_runtime_prefix=`"$PCB_BREW" --prefix 2>/dev/null`
        AS_IF([test -f "$pcb_runtime_prefix/share/glib-2.0/schemas/gschemas.compiled"], [
          pcb_gtk_schema_dir="$pcb_runtime_prefix/share/glib-2.0/schemas"
        ])
      ])
      ;;
  esac
  AC_DEFINE_UNQUOTED([PCB_GTK_SCHEMA_DIR], ["$pcb_gtk_schema_dir"],
    [Fallback GTK schema directory for standalone Homebrew launches.])
  AC_MSG_NOTICE([GTK runtime schema fallback: $pcb_gtk_schema_dir])
])
