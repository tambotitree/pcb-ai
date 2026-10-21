#!/usr/bin/env python3
"""Inventory tracked source/build inputs while preserving human review notes."""

import collections
import hashlib
import json
import pathlib
import re
import subprocess

from version import read_version

LANGUAGES = {
  '.c': 'C', '.h': 'C header', '.cc': 'C++', '.cpp': 'C++',
  '.y': 'Yacc', '.l': 'Lex', '.py': 'Python', '.sh': 'Shell',
  '.pl': 'Perl', '.pm': 'Perl', '.tcl': 'Tcl', '.scm': 'Scheme',
  '.m4': 'M4', '.ac': 'Autoconf', '.am': 'Automake', '.awk': 'Awk',
  '.sed': 'Sed', '.res': 'Menu resource', '.ui': 'GTK Builder',
  '.glade': 'Glade', '.rc': 'Resource',
}
CLUES = {
  'gtk': r'\b(?:gtk_[A-Za-z0-9_]+|Gtk[A-Za-z0-9_]+)\b',
  'gdk': r'\b(?:gdk_[A-Za-z0-9_]+|Gdk[A-Za-z0-9_]+)\b',
  'gtkglext': r'\b(?:GdkGL[A-Za-z0-9_]*|gtk_gl_[A-Za-z0-9_]+|'
               r'gtk_widget_(?:get_gl|set_gl)[A-Za-z0-9_]*|gtkglext)\b|gtk/gtkgl.h',
  'legacy_drawing': r'\b(?:GdkGC|GdkPixmap|gdk_draw_[A-Za-z0-9_]+|'
                    r'gdk_gc_[A-Za-z0-9_]+|expose_event)\b',
  'widget_fields': r'->(?:window|allocation|style)\b',
  'legacy_widgets': r'\bgtk_(?:[hv]box_new|[hv]paned_new|table_[A-Za-z0-9_]+|'
                    r'vseparator_new|hseparator_new|tooltips_[A-Za-z0-9_]+)\b',
  'events': r'\b(?:GdkEvent[A-Za-z0-9_]*|button_press_event|'
            r'button_release_event|motion_notify_event|key_press_event)\b',
  'icons': r'\b(?:gtk_icon_[A-Za-z0-9_]+|gtk_stock_[A-Za-z0-9_]+|GTK_STOCK_[A-Z_]+)\b',
  'internationalization': r'\b(?:intltool[A-Za-z0-9_-]*|IT_PROG_INTLTOOL|'
                          r'AM_GNU_GETTEXT[A-Z_]*|gettext|ENABLE_NLS)\b',
  'guile': r'\b(?:scm_[A-Za-z0-9_]+|SCM|guile)\b',
  'python2_clues': r'\b(?:xrange|raw_input|iteritems|itervalues|iterkeys)\b|'
                   r'^\s*print\s+[^\(\s]',
}


def source_language(path, content):
  """Classify source/build inputs by suffix, template name or script shebang."""
  name = pathlib.PurePosixPath(path)
  if name.suffix == '.in':
    name = name.with_suffix('')
  if name.suffix in LANGUAGES:
    return LANGUAGES[name.suffix]
  if name.name in ('Makefile', 'GNUmakefile'):
    return 'Make'
  first = content.splitlines()[0] if content else ''
  if first.startswith('#!'):
    for pattern, language in (('python', 'Python'), ('perl', 'Perl'),
                              ('wish|tclsh', 'Tcl'), ('(?:ba|k|z)?sh', 'Shell')):
      if re.search(pattern, first):
        return language
  return None


def source_area(path):
  """Assign a coarse navigation group without claiming dependency isolation."""
  for prefix, area in (
      ('src/hid/gtk/', 'GTK GUI'), ('src/hid/ps/', 'PostScript exporter'),
      ('src/hid/', 'Other HID/exporter'), ('src/', 'Core'),
      ('gts/', 'Bundled GTS'), ('tests/', 'Tests'), ('tools/', 'Tools')):
    if path.startswith(prefix):
      return area
  return 'Build/data/support'


def file_entry(path, data, previous):
  """Scan one input, retaining review metadata and detecting subsequent edits."""
  content = data.decode('utf-8', errors='replace')
  language = source_language(path, content)
  if language is None:
    return None
  digest = hashlib.sha256(data).hexdigest()
  review = dict(previous.get('review', {
    'status': 'unreviewed', 'reviewed_sha256': None, 'notes': '', 'checks': [],
  }))
  if review.get('reviewed_sha256') and review['reviewed_sha256'] != digest:
    review['status'] = 'needs_review'
  clues = {}
  for category, pattern in CLUES.items():
    # The scanner's pattern literals are not application migration evidence.
    if path == 'tools/upgrade_inventory.py':
      continue
    if category == 'python2_clues' and language != 'Python':
      continue
    hits = [{'line': index, 'matches': sorted(set(re.findall(pattern, line)))}
            for index, line in enumerate(content.splitlines(), 1)
            if re.search(pattern, line)]
    if hits:
      clues[category] = hits
  return {
    'path': path, 'language': language, 'area': source_area(path),
    'sha256': digest, 'lines': len(content.splitlines()),
    'clues': clues, 'review': review,
    'versions': previous.get('versions', {
      'first_seen_project_version': None, 'scanned_project_version': None,
      'conversions': [],
    }),
  }


def write_inventory(path, data):
  """Write generated JSON with one file record per line for reviewable diffs."""
  lines = []
  for key, value in data.items():
    label = '  ' + json.dumps(key) + ': '
    if key in ('files', 'removed_files'):
      records = [json.dumps(entry, separators=(',', ':')) for entry in value]
      lines.append(label + '[\n' + ',\n'.join('    ' + record for record in records) + '\n  ]')
    else:
      lines.append(label + json.dumps(value, separators=(',', ':')))
  path.write_text('{\n' + ',\n'.join(lines) + '\n}\n')


def main():
  """Scan tracked inputs, preserving prior entries and writing deterministic JSON."""
  root = pathlib.Path(__file__).resolve().parents[1]
  output = root / 'upgrade_data' / 'source_inventory.json'
  old = json.loads(output.read_text()) if output.exists() else {}
  previous = {entry['path']: entry for entry in old.get('files', [])}
  paths = subprocess.check_output(
    ['git', '-C', str(root), 'ls-files', '-z']).decode().split('\0')
  # Include this uncommitted tool during the first inventory as well.
  paths = sorted(set(paths) | {'tools/upgrade_inventory.py', 'tools/conversion.py',
    'tools/python_conversion.py', 'tools/autoconf_conversion.py', 'bootstrap.sh',
    'tools/c_conversion.py', 'tools/scheme_conversion.py',
    'tools/c_signature_conversion.py', 'm4/pcb-macos.m4',
    'tools/gtk3_build_conversion.py', 'tools/struct_macro_conversion.py', 'tools/gtk_api_conversion.py',
    'tools/test_gtk_api_conversion.py', 'tools/timestamp_conversion.py', 'tools/visibility_renderer_conversion.py', 'tools/test_visibility_renderer.py',
    'tools/test_visibility_renderer.c', 'tools/drawing_conversion.py',
    'tools/drawing_backend_rules.py', 'tools/drawing_ui_rules.py', 'tools/menu_conversion.py', 'tools/gtype_conversion.py', 'tools/menu_actions_conversion.py', 'tools/selector_actions_conversion.py', 'tools/dialog_layout_conversion.py', 'src/hid/gtk/pcb-grid.h', 'tools/callback_conversion.py', 'tools/pointer_conversion.py', 'src/hid/gtk/pcb-pointer.h', 'tools/hid_initializer_conversion.py', 'tools/hid_bounds_conversion.py', 'tools/hid_flags_conversion.py', 'tools/benchmark_conversion.py', 'tools/combo_conversion.py', 'tools/desktop_hints_conversion.py', 'tools/array_loop_conversion.py', 'tools/dialog_stock_conversion.py', 'tools/reverse_loop_conversion.py', 'tools/layer_loop_conversion.py',
    'tools/render_cache_conversion.py', 'tools/sentinel_audit.py',
    'tools/topbar_appearance_conversion.py', 'tools/xpm_stream_conversion.py',
    'src/hid/gtk/pcb-cairo.h', 'src/hid/gtk/pcb-cairo.c'})
  project_version = read_version((root / 'configure.ac').read_text())
  entries = []
  for path in paths:
    if not path or path.startswith('upgrade_data/'):
      continue
    source = root / path
    if source.is_symlink() or not source.is_file():
      continue
    data = source.read_bytes()
    if b'\0' in data:
      continue
    entry = file_entry(path, data, previous.get(path, {}))
    if entry:
      entry['versions']['scanned_project_version'] = project_version
      if entry['versions']['first_seen_project_version'] is None:
        entry['versions']['first_seen_project_version'] = project_version
      entries.append(entry)
  present = {entry['path'] for entry in entries}
  removed = {entry['path']: entry for entry in old.get('removed_files', [])}
  removed.update({path: entry for path, entry in previous.items() if path not in present})
  for path in present:
    removed.pop(path, None)
  result = {
    'schema_version': 2,
    'project_version': project_version,
    'scope': 'Tracked text source/build inputs plus the inventory scanner; current working files',
    'baseline_tag': '0.1.0-20261020',
    'summary': {
      'files': len(entries),
      'languages': dict(sorted(collections.Counter(e['language'] for e in entries).items())),
      'areas': dict(sorted(collections.Counter(e['area'] for e in entries).items())),
      'files_with_clue': {key: sum(key in entry['clues'] for entry in entries) for key in CLUES},
    },
    'files': entries,
    'removed_files': [removed[path] for path in sorted(removed)],
  }
  output.parent.mkdir(exist_ok=True)
  write_inventory(output, result)
  print(json.dumps(result['summary'], indent=2))


if __name__ == '__main__':
  main()
