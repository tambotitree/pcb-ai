"""Exact memory-only patches for explicitly supplied gEDA design snapshots."""
import base64
import difflib
import hashlib
import re

LIMIT = 4 * 1024 * 1024
HUNK = re.compile(r'@@ -(\d+)(?:,(\d+))? \+(\d+)(?:,(\d+))? @@[^\n]*\n?')


def digest(text):
    """Hash the exact UTF-8 snapshot, including line endings."""
    return hashlib.sha256(text.encode('utf-8')).hexdigest()


def apply_exact(source, diff, path):
    """Apply ordered hunks without fuzz, filesystem access, or shell commands."""
    if not isinstance(diff, str) or len(diff.encode()) > LIMIT or '\0' in diff:
        raise ValueError('Invalid patch size or encoding')
    lines = diff.splitlines(keepends=True)
    if len(lines) < 3 or lines[:2] != [f'--- a/{path}\n', f'+++ b/{path}\n']:
        raise ValueError('Patch headers must name the supplied design document exactly')
    original = source.splitlines(keepends=True)
    result, cursor, index = [], 0, 2
    while index < len(lines):
        match = HUNK.fullmatch(lines[index])
        if not match:
            raise ValueError('Expected a unified-diff hunk')
        old_start, old_count, new_start, new_count = (int(v) if v is not None else 1 for v in match.groups())
        start = old_start - 1 if old_count else old_start
        if start < cursor or start > len(original):
            raise ValueError('Overlapping or out-of-range patch hunk')
        result.extend(original[cursor:start])
        cursor = start
        if len(result) != (new_start - 1 if new_count else new_start):
            raise ValueError('Inconsistent new hunk position')
        index += 1
        removed = added = 0
        while index < len(lines) and not lines[index].startswith('@@ '):
            line = lines[index]
            if not line or line[0] not in ' +-':
                raise ValueError('Unsupported hunk line; newline-terminated designs required')
            marker, content = line[0], line[1:]
            if marker in ' -':
                if cursor >= len(original) or original[cursor] != content:
                    raise ValueError('Patch context does not match the supplied snapshot')
                cursor += 1
                removed += 1
            if marker in ' +':
                result.append(content)
                added += 1
            index += 1
        if (removed, added) != (old_count, new_count):
            raise ValueError('Patch hunk counts do not match')
    result.extend(original[cursor:])
    updated = ''.join(result)
    if updated == source or len(updated.encode()) > LIMIT:
        raise ValueError('Patch is empty or resulting design is too large')
    return updated


def replacement_diff(source, replacements, path):
    """Build an exact diff from one-based ranges in the hashed source snapshot."""
    if not isinstance(replacements, list) or not 1 <= len(replacements) <= 256:
        raise ValueError('Expected one to 256 numbered replacements')
    lines = source.splitlines(keepends=True)
    edits = []
    for replacement in replacements:
        if not isinstance(replacement, dict):
            raise ValueError('Invalid numbered replacement')
        start, end, text = (replacement.get(key) for key in ('start_line', 'end_line', 'text'))
        if (type(start) is not int or type(end) is not int or not 1 <= start <= end <= len(lines)):
            raise ValueError('Replacement line range is outside the supplied snapshot')
        if (not isinstance(text, str) or '\0' in text or len(text.encode()) > LIMIT
                or (text and not text.endswith('\n'))):
            raise ValueError('Replacement text must be bounded and newline-terminated')
        edits.append((start - 1, end, text))
    result, cursor = [], 0
    for start, end, text in sorted(edits):
        if start < cursor:
            raise ValueError('Numbered replacement ranges overlap')
        result.extend(lines[cursor:start])
        result.append(text)
        cursor = end
    result.extend(lines[cursor:])
    updated = ''.join(result)
    if updated == source or len(updated.encode()) > LIMIT:
        raise ValueError('Replacement is empty or resulting design is too large')
    return ''.join(difflib.unified_diff(lines, updated.splitlines(keepends=True),
                                      fromfile='a/' + path, tofile='b/' + path))


def stage(patches, documents):
    """Validate all proposed documents before returning any staged contents."""
    if not isinstance(patches, list) or not 1 <= len(patches) <= 2:
        raise ValueError('Expected one or two design patches')
    staged, seen = [], set()
    for patch in patches:
        if not isinstance(patch, dict):
            raise ValueError('Invalid patch object')
        path = patch.get('path')
        if (not isinstance(path, str) or path not in documents or path in seen
                or not path.endswith(('.pcb', '.sch'))):
            raise ValueError('Only explicitly supplied .pcb and .sch documents may be patched')
        source = documents[path]
        if patch.get('base_sha256') != digest(source):
            raise ValueError('Patch base hash does not match the supplied design')
        if 'diff' in patch and 'replacements' in patch:
            raise ValueError('Use either a diff or numbered replacements, not both')
        diff = replacement_diff(source, patch['replacements'], path) if 'replacements' in patch else patch.get('diff')
        updated = apply_exact(source, diff, path)
        if path.endswith('.pcb') and re.search(r'^\s*Symbol(?:Line)?\s*[\[(]', updated, re.MULTILINE):
            raise ValueError('Embedded font edits are unavailable in compact PCB context')
        staged.append((path, diff, updated))
        seen.add(path)
    return staged


def encode(staged):
    """Encode validated contents for the local GTK helper envelope."""
    fields = [f'patch_count={len(staged)}']
    for index, (path, diff, updated) in enumerate(staged):
        for key, value in (('path', path), ('diff', diff), ('content', updated)):
            fields.append(f'patch_{index}_{key}=' + base64.b64encode(value.encode()).decode())
    return '\n'.join(fields) + '\n'
