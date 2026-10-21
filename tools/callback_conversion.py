#!/usr/bin/env python3
"""Preserve callback parameter types while marking compiler-confirmed unused inputs."""

import json
import re
from pathlib import Path

from c_conversion import NONCODE
from conversion import main

TABLE = Path(__file__).resolve().parents[1] / 'upgrade_data/callback_parameters.json'


def definitions(text):
    """Locate top-level C function signatures and bodies, masking comments/literals."""
    masked = NONCODE.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), text)
    pattern = re.compile(r'^(?:static[ \t]+\w+[ \t]+)?(\w+)\s*\(([^;{}]*)\)\s*\n\{', re.M)
    for match in pattern.finditer(masked):
        brace = match.end() - 1
        end = brace + 1
        depth = 1
        while depth:
            if masked[end] == '{': depth += 1
            if masked[end] == '}': depth -= 1
            end += 1
        yield match, masked[brace + 1:end - 1]


def transform(source, root, relative, tool):
    """Apply recorded annotations only to unchanged, unused formal parameters."""
    rules = [r for r in json.loads(TABLE.read_text())['parameters'] if r['path'] == relative]
    if not rules:
        return source, 'No compiler-confirmed callback inputs selected.'
    text = source.decode()
    functions = {m[1]: (m, body) for m, body in definitions(text)}
    edits = []
    for rule in rules:
        match, body = functions[rule['function']]
        name = rule['parameter']
        tokens = list(re.finditer(r'\b' + re.escape(name) + r'\b', match[2]))
        if len(tokens) != 1:
            raise ValueError('Ambiguous parameter: ' + str(rule))
        pos = match.start(2) + tokens[0].end()
        if re.match(r'\s+G_GNUC_UNUSED\b', text[pos:]):
            continue
        body = re.sub(r'(?:->|\.)\s*' + re.escape(name) + r'\b', '', body)
        if re.search(r'\b' + re.escape(name) + r'\b', body):
            raise ValueError('Parameter is referenced; manual review required: ' + str(rule))
        edits.append((pos, ' G_GNUC_UNUSED'))
    for pos, replacement in sorted(edits, reverse=True):
        text = text[:pos] + replacement + text[pos:]
    if relative == 'src/hid/gtk/gtkhid-main.c':
        text = text.replace('''  ghid_block_hook_prepare,
  ghid_block_hook_check,
  ghid_block_hook_dispatch,
  NULL // No destroy notification''', '''  .prepare = ghid_block_hook_prepare,
  .check = ghid_block_hook_check,
  .dispatch = ghid_block_hook_dispatch,
  .finalize = NULL,
  .closure_callback = NULL,
  .closure_marshal = NULL''')
        text = text.replace('''  hidval data = ((BlockHookSource *)source)->user_data;
  ((BlockHookSource *)source)->func( data );''', '''  hidval data = ((BlockHookSource *)source)->user_data;
  /* This hook runs during prepare; it requests no additional wakeup timer. */
  *timeout = -1;
  ((BlockHookSource *)source)->func( data );''')
    if relative == 'src/hid/gtk/gui-drc-window.c':
        text = re.sub(r'(ghid_violation_renderer_get_size\s*\(GtkCellRenderer\s*\*cell,\s*'
                      r'GtkWidget\s*\*widget,\s*)GdkRectangle', r'\1const GdkRectangle', text)
    return text.encode(), f'{len(edits)} retained API inputs explicitly unused; types unchanged.'


if __name__ == '__main__':
    raise SystemExit(main('callbacks', transform, 'python3'))
