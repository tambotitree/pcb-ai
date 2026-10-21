#!/usr/bin/env python3
"""Migrate PCB's file-change timestamps using gschem's timespec approach."""

from conversion import main

CHECK = '''/*! \brief Detect a newer disk revision once per observed timestamp. */
static bool
check_externally_modified (void)
{
  struct stat file_stat;
  struct timespec current;

  if (PCB->Filename == NULL || !ghidgui->has_known_mtime ||
      stat (PCB->Filename, &file_stat) != 0)
    return false;

#if defined(HAVE_STRUCT_STAT_ST_MTIM)
  current = file_stat.st_mtim;
#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC)
  current = file_stat.st_mtimespec;
#else
#error "No supported nanosecond file timestamp member"
#endif

  if (current.tv_sec == ghidgui->last_seen_mtime.tv_sec &&
      current.tv_nsec == ghidgui->last_seen_mtime.tv_nsec)
    return false;

  ghidgui->last_seen_mtime = current;
  return current.tv_sec > ghidgui->our_mtime.tv_sec ||
         (current.tv_sec == ghidgui->our_mtime.tv_sec &&
          current.tv_nsec > ghidgui->our_mtime.tv_nsec);
}

'''

UPDATE = '''/*! \brief Record the saved board timestamp, or clear its known state. */
static void
update_board_mtime_from_disk (void)
{
  struct stat file_stat;

  ghidgui->has_known_mtime = false;
  ghidgui->our_mtime = (struct timespec) {0, 0};
  ghidgui->last_seen_mtime = ghidgui->our_mtime;
  if (PCB->Filename == NULL || stat (PCB->Filename, &file_stat) != 0)
    return;

#if defined(HAVE_STRUCT_STAT_ST_MTIM)
  ghidgui->our_mtime = file_stat.st_mtim;
#elif defined(HAVE_STRUCT_STAT_ST_MTIMESPEC)
  ghidgui->our_mtime = file_stat.st_mtimespec;
#else
#error "No supported nanosecond file timestamp member"
#endif
  ghidgui->last_seen_mtime = ghidgui->our_mtime;
  ghidgui->has_known_mtime = true;
}

'''


def replace_function(text, name, replacement):
  """Replace the known timestamp function, refusing unexpected body content."""
  prefix = 'static ' + ('bool' if name == 'check_externally_modified' else 'void')
  marker = prefix + '\n' + name + ' (void)\n{'
  if replacement in text:
    return text
  if text.count(marker) != 1:
    raise ValueError('Expected one timestamp function: ' + name)
  start = text.index(marker)
  # These two original functions have no nested blocks; require that property.
  end = text.index('\n}\n', start) + 3
  original = text[start:end]
  if original.count('{') != 1 or 'g_file_info_get_modification_time' not in original:
    raise ValueError('Unexpected timestamp implementation: ' + name)
  return text[:start] + replacement.rstrip() + '\n' + text[end:]


def transform(source, root, relative, tool):
  """Convert the fields, both consumers and platform checks as one selected pass."""
  text = source.decode()
  if relative == 'src/hid/gtk/gui.h':
    old = '  GTimeVal our_mtime;\n  GTimeVal last_seen_mtime;'
    new = ('  bool has_known_mtime;\n  struct timespec our_mtime;\n'
           '  struct timespec last_seen_mtime;')
    if old in text:
      text = text.replace(old, new)
    elif new not in text:
      raise ValueError('Unexpected timestamp field declarations')
    if '#include <time.h>' not in text:
      text = text.replace('#include <sys/stat.h>', '#include <sys/stat.h>\n#include <time.h>')
  elif relative == 'src/hid/gtk/gui-top-window.c':
    text = replace_function(text, 'check_externally_modified', CHECK)
    text = replace_function(text, 'update_board_mtime_from_disk', UPDATE)
  elif relative == 'configure.ac':
    probe = '''dnl File timestamps: Linux st_mtim or macOS st_mtimespec, as in gschem.
AC_CHECK_MEMBERS([struct stat.st_mtim, struct stat.st_mtimespec], [], [],
                [[#include <sys/stat.h>]])
'''
    if probe not in text:
      marker = 'AC_PROG_CXX\n'
      if text.count(marker) != 1:
        raise ValueError('Expected compiler configuration marker')
      text = text.replace(marker, marker + probe)
  else:
    return source, 'Not a timestamp migration input.'
  return text.encode(), 'timespec fields, explicit known-state, platform checks and comparisons.'


if __name__ == '__main__':
  raise SystemExit(main('timestamps', transform, 'python3'))
