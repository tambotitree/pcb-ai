#!/usr/bin/env python3
"""Give HID numeric bounds explicit integer, real and coordinate representations."""

import re

from conversion import main
from hid_initializer_conversion import closing_brace, mask, split_fields


def bound_record(record):
    """Select the bound member from the attribute's explicit HID type."""
    fields = split_fields(record[1:-1])
    type_field = next((f for f in fields if re.search(r'\.type\s*=', mask(f))), None)
    if type_field is None:
        raise ValueError('Attribute lacks a designated type')
    type_name = re.search(r'\.type\s*=\s*(HID_\w+)\s*$', type_field)
    if not type_name:
        raise ValueError('Unrecognized HID type expression')
    kind = type_name[1]
    member = ('real_value' if kind in ('HID_Real', 'HID_Mixed') else
              'coord_value' if kind == 'HID_Coord' else 'int_value')
    result = []
    for field in fields:
        match = re.search(r'\.(min_val|max_val)\s*=\s*', mask(field))
        if match:
            value = field[match.end():]
            if not value.lstrip().startswith('{'):
                field = field[:match.end()] + '{ .' + member + ' = ' + value.rstrip() + ' }' + value[len(value.rstrip()):]
        result.append(field)
    return '{' + ','.join(result) + '}'


def transform(source, root, relative, tool):
    """Convert bound declarations, all attribute tables and typed GTK consumers."""
    if not relative.startswith('src/'):
        return source, 'Outside HID source scope.'
    text = source.decode('latin-1')
    if relative == 'src/hid.h' and 'int min_val;' in text:
        old = '    int min_val; /*!< For integer and real */\n    int max_val; /*!< For integer and real */'
        if old not in text: raise ValueError('HID bound declaration changed')
        text = text.replace(old, '''    HID_Attr_Bound min_val; /*!< Member selected by attribute type. */
    HID_Attr_Bound max_val; /*!< Member selected by attribute type. */''')
        marker = '  typedef struct\n  {\n    char *name;\n#define ATTR_UNDOCUMENTED'
        if marker not in text: raise ValueError('HID attribute insertion point changed')
        text = text.replace(marker, '''  /* Bounds use the same numeric category as the attribute's default value. */
  typedef union
  {
    int int_value;
    double real_value;
    Coord coord_value;
  } HID_Attr_Bound;

''' + marker)
    if relative.endswith('.c'):
        masked = mask(text)
        edits = []
        for match in re.finditer(r'\bHID_Attribute\s+\w+\s*\[[^]]*\]\s*=\s*\{', masked):
            stop = closing_brace(masked, match.end() - 1)
            pos = match.end()
            while pos < stop:
                if masked[pos] == '{':
                    end = closing_brace(masked, pos)
                    edits.append((pos, end + 1, bound_record(text[pos:end + 1])))
                    pos = end + 1
                else: pos += 1
        for start, end, candidate in sorted(edits, reverse=True):
            text = text[:start] + candidate + text[end:]
    if relative == 'src/hid/gtk/gui-dialog-print.c':
        # Each case has already been checked against the corresponding widget API.
        cases = list(re.finditer(r'case (HID_\w+):', text))
        edits = []
        for i, case in enumerate(cases):
            end = cases[i + 1].start() if i + 1 < len(cases) else len(text)
            member = {'HID_Integer': 'int_value', 'HID_Coord': 'coord_value',
                      'HID_Real': 'real_value', 'HID_Mixed': 'real_value'}.get(case[1])
            if not member: continue
            segment = text[case.end():end]
            segment = re.sub(r'\b(attrs\[j\]\.(?:min_val|max_val))\b(?!\.)',
                             r'\1.' + member, segment)
            edits.append((case.end(), end, segment))
        for start, end, candidate in sorted(edits, reverse=True):
            text = text[:start] + candidate + text[end:]
    return text.encode('latin-1'), 'HID bounds retain their numeric type; compile pending.'


if __name__ == '__main__':
    raise SystemExit(main('hid-bounds', transform, 'python3'))
