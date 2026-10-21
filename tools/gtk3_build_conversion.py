#!/usr/bin/env python3
"""Select GTK3 for the experimental GUI build without mixing in gtkglext/GTK2."""

from conversion import main


def transform(source, root, relative, tool):
  """Replace known configure dependency checks; reject unexpected input shapes."""
  if relative != 'configure.ac':
    raise ValueError('This pass is defined only for configure.ac')
  text = source.decode()
  replacements = (
    ('PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.18.0, ,',
     'PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.24.0, ,'),
    ('Cannot find gtk+ >= 2.18.0, install it and rerun ./configure',
     'Cannot find GTK3 >= 3.24.0, install it and rerun ./configure'),
    ('GTK_VERSION=`$PKG_CONFIG gtk+-2.0 --modversion`',
     'GTK_VERSION=`$PKG_CONFIG gtk+-3.0 --modversion`'),
  )
  for old, new in replacements:
    if text.count(old) == 1 and new not in text:
      text = text.replace(old, new)
    elif old not in text and text.count(new) == 1:
      continue
    else:
      raise ValueError('Unexpected configure pattern: ' + old)
  marker = '\t\t# Check for GtkGLExt\n'
  guard = '''\t\t# The legacy gtkglext backend depends on GTK2; never link it into GTK3.
        AC_MSG_ERROR([GTK3 OpenGL backend is not ported; configure with --disable-gl])
'''
  if guard not in text:
    if text.count(marker) != 1:
      raise ValueError('Expected one legacy gtkglext configuration block')
    text = text.replace(marker, guard + marker)
  return text.encode(), ('GTK3 dependency selected for the experimental GUI build; '
                         'legacy GL configuration rejected. This is not a completed GUI port.')


if __name__ == '__main__':
  raise SystemExit(main('gtk3-build', transform, 'python3'))
