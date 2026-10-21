"""Offline regression for exact, scoped design patches and JSON transport."""
import difflib
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'ai'))
import design_patch
import assist_protocol


class DesignPatchTests(unittest.TestCase):
    """No filesystem mutations, credentials, model calls, or network access."""

    def proposal(self, source='a\nb\nc\n', updated='a\nB\nc\n', path='current.pcb'):
        """Construct an exact patch as a model would return it."""
        return {'path': path, 'base_sha256': design_patch.digest(source),
                'diff': ''.join(difflib.unified_diff(source.splitlines(True), updated.splitlines(True),
                                                  fromfile='a/' + path, tofile='b/' + path))}

    def test_exact_patch_and_envelope(self):
        """A validated patch returns staged content; the source stays untouched."""
        source = 'a\nb\nc\n'
        patch = self.proposal()
        result = design_patch.stage([patch], {'current.pcb': source})
        self.assertEqual(result[0][2], 'a\nB\nc\n')
        output = assist_protocol.encode_reply(json.dumps({'reply': 'Change B', 'patches': [patch]}), {'board': source})
        self.assertIn('patch_count=1\n', output)
        self.assertEqual(source, 'a\nb\nc\n')

    def test_explicit_and_legacy_sections(self):
        """A proposal embedded in prose still reaches exact patch validation."""
        proposal = json.dumps({'reply': 'Round pads', 'patches': [self.proposal()]})
        for wrapped in ('<pcb-ai-proposal>' + proposal + '</pcb-ai-proposal>',
                        'Review this:\n```json\n' + proposal + '\n```\nThen press Act.'):
            output = assist_protocol.encode_reply(wrapped, {'board': 'a\nb\nc\n'})
            self.assertIn('patch_count=1\n', output)
        missing_summary = json.dumps({'patches': [self.proposal()]})
        self.assertIn('patch_count=1\n', assist_protocol.encode_reply(
            '<pcb-ai-proposal>' + missing_summary + '</pcb-ai-proposal>', {'board': 'a\nb\nc\n'}))

    def test_invalid_or_ambiguous_sections(self):
        """Tags do not bypass JSON, exact context or document-scope checks."""
        for text in ('<pcb-ai-proposal>{bad}</pcb-ai-proposal>',
                     '<pcb-ai-proposal>{}',
                     '<pcb-ai-proposal>{}</pcb-ai-proposal>' * 2,
                     '```json\n{}\n```\n```json\n{}\n```'):
            with self.assertRaises(ValueError):
                assist_protocol.encode_reply(text, {'board': 'a\nb\nc\n'})
        patch = self.proposal()
        patch['base_sha256'] = 'stale'
        with self.assertRaisesRegex(ValueError, 'hash'):
            assist_protocol.encode_reply('<pcb-ai-proposal>' + json.dumps(
                {'reply': 'Bad base', 'patches': [patch]}) + '</pcb-ai-proposal>', {'board': 'a\nb\nc\n'})

    def test_numbered_replacements_generate_exact_diff(self):
        """Host retains original tabs/context and resolves ranges before edits."""
        source = 'PCB[]\n\tPad[old]\n\n\tPad[other]\n)\n'
        patch = {'path': 'current.pcb', 'base_sha256': design_patch.digest(source),
                 'replacements': [{'start_line': 4, 'end_line': 4, 'text': '\tPad[new2]\n'},
                                  {'start_line': 2, 'end_line': 2, 'text': '\tPad[new1]\n# annotation\n'}]}
        staged = design_patch.stage([patch], {'current.pcb': source})
        self.assertEqual(staged[0][2], 'PCB[]\n\tPad[new1]\n# annotation\n\n\tPad[new2]\n)\n')
        self.assertIn('-\tPad[old]\n', staged[0][1])
        self.assertEqual(design_patch.apply_exact(source, staged[0][1], 'current.pcb'), staged[0][2])

    def test_numbered_replacement_failures(self):
        """Bad ranges, overlaps, mixed diffs and stale bases never stage."""
        source = 'a\nb\nc\n'
        good = {'start_line': 2, 'end_line': 2, 'text': 'B\n'}
        for edits in ([dict(good, start_line=True)], [dict(good, end_line=99)],
                      [dict(good, text='no newline')], [good, good], []):
            with self.assertRaises(ValueError):
                design_patch.stage([{'path': 'current.pcb', 'base_sha256': design_patch.digest(source),
                                     'replacements': edits}], {'current.pcb': source})
        patch = dict(self.proposal(), replacements=[good])
        with self.assertRaisesRegex(ValueError, 'either'):
            design_patch.stage([patch], {'current.pcb': source})
        patch.pop('diff')
        patch['base_sha256'] = 'stale'
        with self.assertRaisesRegex(ValueError, 'hash'):
            design_patch.stage([patch], {'current.pcb': source})

    def test_stale_and_fuzzy_rejected(self):
        """Neither stale hashes nor near-matching context may be accepted."""
        for field, value in (('base_sha256', '0' * 64), ('diff', self.proposal()['diff'].replace('-b', '-x'))):
            patch = self.proposal()
            patch[field] = value
            with self.assertRaises(ValueError):
                design_patch.stage([patch], {'current.pcb': 'a\nb\nc\n'})

    def test_path_scope(self):
        """Unknown paths, non-design files, duplicates and renames are rejected."""
        for path in ('../current.pcb', '/tmp/current.pcb', '.env', 'other.sch'):
            with self.assertRaises(ValueError):
                design_patch.stage([self.proposal(path=path)], {'current.pcb': 'a\nb\nc\n'})
        patch = self.proposal()
        with self.assertRaises(ValueError):
            design_patch.stage([patch, patch], {'current.pcb': 'a\nb\nc\n'})
        patch['diff'] = patch['diff'].replace('+++ b/current.pcb', '+++ b/renamed.pcb')
        with self.assertRaises(ValueError):
            design_patch.stage([patch], {'current.pcb': 'a\nb\nc\n'})

    def test_font_preserved(self):
        """The compact board context cannot authorize edits to omitted glyphs."""
        source = 'PCB[]\n'
        patch = self.proposal(source, source + "Symbol['A' 100]\n(\n)\n")
        with self.assertRaisesRegex(ValueError, 'font'):
            design_patch.stage([patch], {'current.pcb': source})

    def test_counts_and_empty_patch(self):
        """Malformed line counts and no-op patches cannot stage."""
        for diff in ('--- a/current.pcb\n+++ b/current.pcb\n', self.proposal()['diff'].replace('-1,3', '-1,9')):
            with self.assertRaises(ValueError):
                design_patch.apply_exact('a\nb\nc\n', diff, 'current.pcb')

    def test_multiple_hunks_and_insertions(self):
        """Exact positioning works for insertion, removal and separated hunks."""
        source = ''.join(f'{i}\n' for i in range(30))
        updated = 'start\n' + source.replace('5\n', 'five\n').replace('27\n', '') + 'end\n'
        patch = self.proposal(source, updated)
        self.assertEqual(design_patch.stage([patch], {'current.pcb': source})[0][2], updated)

    def test_atomic_and_mutually_exclusive(self):
        """All documents validate before return; native actions cannot mix in."""
        first = self.proposal()
        second = self.proposal(path='paired.sch')
        second['base_sha256'] = 'invalid'
        with self.assertRaises(ValueError):
            design_patch.stage([first, second], {'current.pcb': 'a\nb\nc\n', 'paired.sch': 'a\nb\nc\n'})
        with self.assertRaises(ValueError):
            assist_protocol.encode_reply(json.dumps({'reply': 'Ambiguous', 'patches': [first],
                'action': {'operation': 'scale_selected_pad_width', 'percent': 80}}),
                {'board': 'a\nb\nc\n', 'selection': 'pad id=1'})


if __name__ == '__main__':
    unittest.main()
