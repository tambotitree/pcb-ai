#!/usr/bin/env python3
"""Check table-driven GTK transformations and intentionally deferred API changes."""

import json
import unittest

from gtk_api_conversion import transform


class RuleTests(unittest.TestCase):
  """Exercise rewrite scope, literal preservation and coordinated-change boundaries."""

  def test_typed_fields(self):
    """Only the known adjustment pair receives the explicit GTK type."""
    source = b'GtkObject *h_adjustment, *v_adjustment;\nGtkObject *other;\n'
    result, _ = transform(source, None, 'src/hid/gtk/gui.h', None)
    self.assertEqual(result, b'GtkAdjustment *h_adjustment, *v_adjustment;\nGtkObject *other;\n')
    self.assertEqual(transform(result, None, 'src/hid/gtk/gui.h', None)[0], result)
    self.assertEqual(transform(source, None, 'src/unrelated.h', None)[0], source)

  def test_comments_and_strings(self):
    """Do not rewrite samples or consume comments inside expressions."""
    source = b'// GTK_OBJECT (dialog)\n"GTK_OBJECT (dialog)";\nGTK_OBJECT (/*keep*/ dialog);\n'
    self.assertEqual(transform(source, None, 'src/hid/gtk/gui-utils.c', None)[0], source)

  def test_scrollbar_arguments(self):
    """Preserve the adjustment expression and add the appropriate orientation."""
    source = b'gtk_hscrollbar_new (GTK_ADJUSTMENT (g->h_adjustment));\n'
    result, _ = transform(source, None, 'src/hid/gtk/gui-top-window.c', None)
    self.assertEqual(result, b'gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT (g->h_adjustment));\n')
    self.assertEqual(transform(result, None, 'src/hid/gtk/gui-top-window.c', None)[0], result)

  def test_deferred_changes(self):
    """Report lifetime/type-sensitive changes without silently renaming them."""
    source = b'GTimeVal t;\ngdk_color_parse (name, &color);\n'
    result, report = transform(source, None, 'src/hid/gtk/gui.h', None)
    self.assertEqual(result, source)
    self.assertTrue(all(item['status'] == 'needs_rule'
                        for item in json.loads(report)['findings']))

  def test_evidence_and_context_ownership(self):
    """Retain compiler hints and gschem's context-specific example together."""
    source = b'cairo_t *cr = gdk_cairo_create (window);\n'
    result, report = transform(source, None,
                               'src/hid/gtk/ghid-cell-renderer-visibility.c', None)
    self.assertEqual(result, source)
    finding = json.loads(report)['findings'][0]
    self.assertTrue(finding['compiler_suggestion'])
    self.assertTrue(finding['gschem_examples'])
    self.assertIn('GTK-provided', finding['target_api'])


if __name__ == '__main__':
  unittest.main()
