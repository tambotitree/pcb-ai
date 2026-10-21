#!/usr/bin/env python3
"""Name HID table fields without changing values, order or callback types."""

import re

from c_conversion import NONCODE
from conversion import main

FIELDS = {
    'HID_Action': ('name', 'need_coord_msg', 'trigger_cb', 'description', 'syntax'),
    'HID_Attribute': ('name', 'help_text', 'type', 'min_val', 'max_val',
                      'default_val', 'enumerations', 'value', 'hash'),
    'HID_Attr_Val': ('int_value', 'str_value', 'real_value', 'coord_value'),
}


def mask(text):
    """Hide comments and literals without changing source offsets."""
    return NONCODE.sub(lambda m: ' ' * len(m[0]), text)


def closing_brace(masked, start):
    """Find the closing brace of a balanced aggregate initializer."""
    depth = 1
    pos = start + 1
    while depth and pos < len(masked):
        if masked[pos] == '{': depth += 1
        if masked[pos] == '}': depth -= 1
        pos += 1
    if depth:
        raise ValueError('Unbalanced initializer')
    return pos - 1


def split_fields(body):
    """Split only aggregate-level commas, retaining nested calls and defaults."""
    masked = mask(body)
    depth = 0
    start = 0
    result = []
    for pos, char in enumerate(masked):
        if char in '({[': depth += 1
        elif char in ')}]': depth -= 1
        elif char == ',' and depth == 0:
            result.append(body[start:pos])
            start = pos + 1
    if body[start:].strip(): result.append(body[start:])
    return result


def aggregate(text, kind):
    """Designate supplied fields; C still zero-initializes omitted members."""
    values = split_fields(text[1:-1])
    if any(re.match(r'\s*\.', mask(value)) for value in values):
        if not all(re.match(r'\s*\.', mask(value)) for value in values):
            raise ValueError('Mixed positional/designated initializer needs review')
        return text
    if not values or len(values) > len(FIELDS[kind]):
        raise ValueError('Unexpected field count for ' + kind)
    output = []
    for field, value in zip(FIELDS[kind], values):
        if kind == 'HID_Attribute' and field == 'default_val':
            a = mask(value).find('{')
            if a < 0:
                raise ValueError('Default value is not an aggregate')
            b = closing_brace(mask(value), a)
            value = value[:a] + aggregate(value[a:b + 1], 'HID_Attr_Val') + value[b + 1:]
        leading = len(value) - len(value.lstrip())
        output.append(value[:leading] + '.' + field + ' = ' + value[leading:])
    return '{' + ','.join(output) + '}'


def value_tokens(text):
    """Compare value tokens while ignoring comments, spacing and field labels."""
    names = '|'.join(sorted({field for fields in FIELDS.values() for field in fields}))
    pattern = re.compile(NONCODE.pattern + r'|\s+|\.(?:' + names + r')\s*=|.', re.S)
    tokens = []
    for match in pattern.finditer(text):
        token = match[0]
        if token.startswith(('/*', '//')) or token.isspace(): continue
        if re.fullmatch(r'\.(?:' + names + r')\s*=', token): continue
        tokens.append(token)
    return tokens


def transform(source, root, relative, tool):
    """Convert every explicit HID_Action/HID_Attribute array in C source."""
    if not relative.startswith('src/') or not relative.endswith('.c'):
        return source, 'Outside C implementation scope.'
    # Some upstream files are not UTF-8. Latin-1 preserves every original byte.
    text = source.decode('latin-1')
    masked = mask(text)
    pattern = re.compile(r'\b(HID_Attribute|HID_Action)\s+\w+\s*\[[^]]*\]\s*=\s*\{')
    edits = []
    tables = 0
    for declaration in pattern.finditer(masked):
        start = declaration.end() - 1
        end = closing_brace(masked, start)
        tables += 1
        pos = start + 1
        while pos < end:
            if masked[pos] == '{':
                stop = closing_brace(masked, pos)
                candidate = aggregate(text[pos:stop + 1], declaration[1])
                if candidate != text[pos:stop + 1]:
                    edits.append((pos, stop + 1, candidate))
                pos = stop + 1
            else:
                pos += 1
    for start, end, candidate in sorted(edits, reverse=True):
        text = text[:start] + candidate + text[end:]
    if value_tokens(text) != value_tokens(source.decode('latin-1')):
        raise ValueError('Initializer value tokens changed: ' + relative)
    return text.encode('latin-1'), f'{tables} tables inspected; {len(edits)} records designated.'


if __name__ == '__main__':
    raise SystemExit(main('hid-initializers', transform, 'python3'))
