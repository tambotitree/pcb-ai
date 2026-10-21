#!/usr/bin/env python3
"""Make the five HID boolean bitfields represent zero and one without sign loss."""

import re

from conversion import main


def transform(source, root, relative, tool):
    """Change only the reviewed boolean flags in the common HID declaration."""
    if relative != 'src/hid.h':
        return source, 'Outside the HID boolean declaration.'
    text = source.decode()
    pattern = r'^([ \t]*)char (gui|printer|exporter|poly_before|poly_after):1;'
    text, count = re.subn(pattern, r'\1unsigned char \2:1;', text, flags=re.M)
    return text.encode(), f'{count} HID boolean flags made unsigned.'


if __name__ == '__main__':
    raise SystemExit(main('hid-flags', transform, 'python3'))
