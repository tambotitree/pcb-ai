#!/usr/bin/env python3
"""Replace legacy GType initializer casts with correctly typed API adapters."""

import re

from conversion import main
from c_conversion import NONCODE


def transform(source, root, relative, tool):
    """Adapt supported GTypeInfo records while retaining typed widget functions."""
    if not relative.startswith('src/hid/gtk/') or not relative.endswith('.c'):
        return source, 'Outside GTK implementation scope.'
    text = source.decode()
    # Repair the earlier rule's extra separator after a trailing comment.
    text = re.sub(r'(,\s*/\* instance_init \*/),', r'\1', text)
    pattern = re.compile(r'((?:static )?const GTypeInfo \w+\s*=\s*\{)(.*?)(\n\s*\};)', re.S)
    edits = []
    count = 0
    for record in pattern.finditer(text):
        body = record[2]
        types = re.findall(r'sizeof\s*\((\w+)\)', body)
        class_cb = re.search(r'\(GClassInitFunc\)\s*(\w+)', body)
        instance_cb = re.search(r'\(GInstanceInitFunc\)\s*(\w+)', body)
        if not class_cb:
            continue
        if len(types) != 2 or 'value_table' in body:
            raise ValueError('Unexpected GTypeInfo layout: ' + relative)
        # Both adapters use the exact signatures required by GLib. The casts
        # convert API-provided object pointers, never incompatible functions.
        callback = class_cb[1]
        adapter = callback + '_adapter'
        bridges = f'''/*! \\brief Adapt GLib class initialization to the typed class. */
static void
{adapter} (gpointer klass, gpointer data)
{{
  (void) data;
  {callback} (({types[0]} *) klass);
}}

'''
        body = body.replace(class_cb[0], adapter)
        if instance_cb:
            callback = instance_cb[1]
            adapter = callback + '_adapter'
            bridges += f'''/*! \\brief Adapt GLib instance initialization to the typed widget. */
static void
{adapter} (GTypeInstance *instance, gpointer klass)
{{
  (void) klass;
  {callback} (({types[1]} *) instance);
}}

'''
            body = body.replace(instance_cb[0], adapter)
        body = body.rstrip()
        if not NONCODE.sub('', body).rstrip().endswith(','):
            body += ','
        body += '\n        NULL /* value_table */'
        starts = list(re.finditer(r'^GType\s+\w+\s*\(\s*(?:void)?\s*\)', text[:record.start()], re.M))
        if not starts:
            raise ValueError('No enclosing get_type definition: ' + relative)
        edits.append((starts[-1].start(), starts[-1].start(), bridges))
        edits.append((record.start(), record.end(), record[1] + body + record[3]))
        count += 1
    for start, end, replacement in sorted(edits, reverse=True):
        text = text[:start] + replacement + text[end:]
    return text.encode(), f'{count} GTypeInfo records adapted; compilation pending.'


if __name__ == '__main__':
    raise SystemExit(main('gtype', transform, 'python3'))
