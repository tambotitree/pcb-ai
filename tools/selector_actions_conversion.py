#!/usr/bin/env python3
"""Convert layer/style action proxies to owned widgets and typed callbacks."""

import re

from conversion import main
from drawing_conversion import replace_function
from menu_actions_conversion import replace_block


ACCEL = '''/*! \\brief Bind a shortcut directly to a selector menu item. */
static void
selector_add_accel (GtkWidget *item, GtkAccelGroup *group, const char *text)
{
  guint key;
  GdkModifierType modifiers;
  gtk_accelerator_parse (text, &key, &modifiers);
  if (key)
    gtk_widget_add_accelerator (item, "activate", group, key, modifiers,
                                GTK_ACCEL_VISIBLE);
}

'''


def layer(text):
    """Replace layer visibility/selection proxies with persistent menu widgets."""
    text = text.replace('GtkRadioAction *action', 'GtkCheckMenuItem *action')
    text = text.replace('GtkToggleAction *action', 'GtkCheckMenuItem *action')
    for line in ('  GtkActionGroup *action_group;\n', '  GSList *radio_group;\n',
                 '  int n_actions;\n', '  GtkToggleAction *view_action;\n',
                 '  GtkRadioAction  *pick_action;\n'):
        text = text.replace(line, '')
    text = text.replace('  gint accel_index;', '  gulong view_handler;\n  gulong pick_handler;\n  gint accel_index;')
    text = re.sub(r'/\* GTK3 retains GtkAction;.*?G_GNUC_BEGIN_IGNORE_DEPRECATIONS\n', '', text, flags=re.S)
    text = text.replace('G_GNUC_END_IGNORE_DEPRECATIONS', '')
    text = text.replace('static void\ng_cclosure_user_marshal', ACCEL + 'static void\ng_cclosure_user_marshal', 1)
    text = replace_function(text, 'free_ldata', '''static void
free_ldata (GHidLayerSelector *ls, struct _layer *ldata)
{
  if (!ldata) return;
  if (ldata->pick_item)
    {
      gtk_widget_destroy (ldata->pick_item);
      g_object_unref (ldata->pick_item);
    }
  gtk_widget_destroy (ldata->view_item);
  g_object_unref (ldata->view_item);
  gtk_tree_row_reference_free (ldata->rref);
  if (ldata->accel_index >= 0)
    ls->accel_available[ldata->accel_index] = TRUE;
  g_free (ldata);
}''')
    text = text.replace('gtk_action_block_activate (GTK_ACTION (ldata->view_action));',
                        'g_signal_handler_block (ldata->view_item, ldata->view_handler);')
    text = text.replace('gtk_action_unblock_activate (GTK_ACTION (ldata->view_action));',
                        'g_signal_handler_unblock (ldata->view_item, ldata->view_handler);')
    text = replace_block(text, '      if (ldata && ldata->pick_action)',
                         '      g_signal_emit (ls, ghid_layer_selector_signals[SELECT_LAYER_SIGNAL]', '''      {
        GtkTreeIter row;
        gboolean valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (ls->list_store), &row);
        while (valid)
          {
            struct _layer *other;
            gtk_tree_model_get (GTK_TREE_MODEL (ls->list_store), &row,
                                STRUCT_COL, &other, -1);
            if (other && other->pick_item)
              {
                g_signal_handler_block (other->pick_item, other->pick_handler);
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (other->pick_item), other == ldata);
                g_signal_handler_unblock (other->pick_item, other->pick_handler);
              }
            valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (ls->list_store), &row);
          }
      }
''')
    text = text.replace('gtk_toggle_action_get_active (action)', 'gtk_check_menu_item_get_active (action)')
    text = text.replace('  if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))', '  if (action)')
    # Selection comes from the tree; programmatic check-state changes emit
    # toggled, not activate, so synchronization does not dispatch commands.
    text = text.replace('      g_signal_handler_block (ls->selection, ls->selection_changed_sig_id);\n', '')
    text = text.replace('      g_signal_handler_unblock (ls->selection, ls->selection_changed_sig_id);\n', '')
    text = text.replace('      g_signal_emit (ls, ghid_layer_selector_signals[SELECT_LAYER_SIGNAL],\n                     0, user_id);\n    }\n}\n\nstatic void\nghid_layer_selector_init', '    }\n}\n\nstatic void\nghid_layer_selector_init', 1)
    text = text.replace('      gtk_tree_selection_select_path (ls->selection, path);',
                        '      g_signal_handler_block (ls->selection, ls->selection_changed_sig_id);\n'
                        '      gtk_tree_selection_select_path (ls->selection, path);\n'
                        '      g_signal_handler_unblock (ls->selection, ls->selection_changed_sig_id);\n'
                        '      selection_changed_cb (ls->selection, ls);\n'
                        '      gtk_tree_path_free (path);')
    text = text.replace('  ls->action_group = gtk_action_group_new ("LayerSelector");\n', '')
    text = text.replace('  ls->n_actions = 0;\n', '').replace('  g_object_unref (ls->action_group);\n', '')
    text = text.replace('  gchar *pname, *vname;\n', '')
    text = text.replace('new_layer = g_malloc (sizeof (*new_layer))', 'new_layer = g_malloc0 (sizeof (*new_layer))')
    text = replace_block(text, '  /* -- Setup new actions -- */', '\n}\n\n/*! \\brief Install', '''  path = gtk_tree_model_get_path (GTK_TREE_MODEL (ls->list_store), &iter);
  new_layer->rref = gtk_tree_row_reference_new (GTK_TREE_MODEL (ls->list_store), path);
  gtk_tree_path_free (path);
  if (selectable)
    {
      new_layer->pick_item = gtk_check_menu_item_new_with_label (name);
      g_object_ref_sink (new_layer->pick_item);
      gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (new_layer->pick_item), TRUE);
      new_layer->pick_handler = g_signal_connect (new_layer->pick_item, "activate", G_CALLBACK (menu_pick_cb), new_layer);
    }
  new_layer->view_item = gtk_check_menu_item_new_with_label (name);
  g_object_ref_sink (new_layer->view_item);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (new_layer->view_item), visible);
  new_layer->view_handler = g_signal_connect (new_layer->view_item, "activate",
                                              G_CALLBACK (menu_view_cb), new_layer);
  for (i = 0; i < 20; ++i)
    if (ls->accel_available[i]) break;
  new_layer->accel_index = i < 20 ? i : -1;
  if (i < 20)
    {
      gchar *pick = g_strdup_printf ("%s%d", i < 10 ? "" : "<Alt>", (i + 1) % 10);
      gchar *view = g_strdup_printf ("<Ctrl>%s%d", i < 10 ? "" : "<Alt>", (i + 1) % 10);
      if (new_layer->pick_item) selector_add_accel (new_layer->pick_item, ls->accel_group, pick);
      selector_add_accel (new_layer->view_item, ls->accel_group, view);
      ls->accel_available[i] = FALSE;
      g_free (pick);
      g_free (view);
    }
''')
    for kind in ('pick', 'view'):
        text = text.replace(f'ldata->{kind}_action', f'ldata->{kind}_item')
        text = re.sub(r'          GtkAction \*action = GTK_ACTION \(ldata->'+kind+r'_item\);\n          ldata->'+kind+r'_item = gtk_action_create_menu_item \(action\);\n', '', text)
    return text


ROUTE_SYNC = '''/*! \\brief Synchronize menu checks and radio buttons without dispatching. */
static void
route_set_active (GHidRouteStyleSelector *rss, struct _route_style *selected)
{
  GtkTreeIter iter;
  gboolean valid;
  rss->syncing = TRUE;
  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (rss->model), &iter);
  while (valid)
    {
      struct _route_style *style;
      gtk_tree_model_get (GTK_TREE_MODEL (rss->model), &iter, DATA_COL, &style, -1);
      if (style)
        {
          gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (style->menu_item), style == selected);
          if (style == selected) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (style->button), TRUE);
        }
      valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (rss->model), &iter);
    }
  if (!selected) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rss->null_button), TRUE);
  rss->active_style = selected;
  rss->syncing = FALSE;
}

/*! \\brief Dispatch a routing menu item through the typed style selector. */
static void
route_menu_cb (GtkCheckMenuItem *item, GHidRouteStyleSelector *rss)
{
  struct _route_style *style = g_object_get_data (G_OBJECT (item), "route-style");
  if (!rss->syncing) ghid_route_style_selector_select_style (rss, style->rst);
}

'''


def route(text):
    """Make routing buttons/menu checks share explicit selection synchronization."""
    text = text.replace('static gint action_count;', '')
    text = text.replace('GtkVBox parent;', 'GtkBox parent;').replace('GtkVBoxClass parent_class;', 'GtkBoxClass parent_class;')
    text = text.replace('static GtkVBox *ghid_route_style_selector_parent_class;', 'static GtkBoxClass *ghid_route_style_selector_parent_class;')
    text = text.replace('GTK_TYPE_VBOX', 'GTK_TYPE_BOX')
    for line in ('  GSList *action_radio_group;\n', '  GtkActionGroup *action_group;\n',
                 '  GtkRadioAction *null_action;\n', '  GtkRadioAction *action;\n', '  gulong sig_id;\n'):
        text = text.replace(line, '')
    text = text.replace('  gint shortcut_key_idx;', '  gboolean syncing;\n  gint shortcut_key_idx;')
    text = text.replace('  gtk_radio_action_set_group (rss->null_action, rss->action_radio_group);\n  rss->action_radio_group = gtk_radio_action_get_group (rss->null_action);\n', '')
    text = text.replace('/* SIGNAL HANDLERS */', ACCEL + ROUTE_SYNC + '/* SIGNAL HANDLERS */', 1)
    text = replace_function(text, 'radio_select_cb', '''static void
radio_select_cb (GtkToggleButton *button, GHidRouteStyleSelector *rss)
{
  struct _route_style *style = g_object_get_data (G_OBJECT (button), "route-style");
  if (!rss->syncing && gtk_toggle_button_get_active (button))
    ghid_route_style_selector_select_style (rss, style->rst);
}''')
    text = text.replace('gtk_action_set_label (GTK_ACTION (style->action), rst->Name);',
                        'gtk_menu_item_set_label (GTK_MENU_ITEM (style->menu_item), rst->Name);\n          gtk_button_set_label (GTK_BUTTON (style->button), rst->Name);')
    text = text.replace('  ghid_route_style_selector_edit_dialog (rss);', '  (void) btn;\n  ghid_route_style_selector_edit_dialog (rss);', 1)
    text = text.replace('ghid_route_style_selector_init (GHidRouteStyleSelector *rss)\n{',
                        'ghid_route_style_selector_init (GHidRouteStyleSelector *rss)\n{\n  gtk_orientable_set_orientation (GTK_ORIENTABLE (rss), GTK_ORIENTATION_VERTICAL);')
    text = text.replace('  g_object_unref (rss->action_group);\n', '')
    text = text.replace('  rss->action_radio_group = NULL;\n', '')
    text = replace_block(text, '  rss->action_group = gtk_action_group_new', '  rss->shortcut_key_idx = 1;', '''  rss->null_button = gtk_radio_button_new (rss->button_radio_group);
  g_object_ref_sink (rss->null_button);
  init_radio_groups (rss);
''')
    text = text.replace('  gchar *action_name = g_strdup_printf ("RouteStyle%d", action_count);\n', '')
    text = replace_block(text, '  new_style->action = gtk_radio_action_new', '\n  gtk_list_store_append', '''  new_style->menu_item = gtk_check_menu_item_new_with_label (data->Name);
  g_object_ref_sink (new_style->menu_item);
  gtk_check_menu_item_set_draw_as_radio (GTK_CHECK_MENU_ITEM (new_style->menu_item), TRUE);
  new_style->button = gtk_radio_button_new_with_label (rss->button_radio_group, data->Name);
  rss->button_radio_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (new_style->button));
''')
    text = replace_block(text, '      gtk_action_set_accel_group', '      g_free (accel);', '      selector_add_accel (new_style->menu_item, rss->accel_group, accel);\n')
    text = replace_block(text, '  g_object_set_data (G_OBJECT (new_style->action)', '  return new_style;', '''  g_object_set_data (G_OBJECT (new_style->menu_item), "route-style", new_style);
  g_object_set_data (G_OBJECT (new_style->button), "route-style", new_style);
  g_signal_connect (new_style->menu_item, "activate", G_CALLBACK (route_menu_cb), rss);
  g_signal_connect (new_style->button, "toggled", G_CALLBACK (radio_select_cb), rss);
  gtk_box_pack_start (GTK_BOX (rss), new_style->button, FALSE, FALSE, 0);

''')
    text = text.replace('      GtkAction *action;\n', '')
    text = text.replace('      action = GTK_ACTION (style->action);\n      style->menu_item = gtk_action_create_menu_item (action);\n', '')
    text = re.sub(r'          g_signal_handler_block \(G_OBJECT \(style->action\), style->sig_id\);.*?          rss->active_style = style;', '          route_set_active (rss, style);', text, flags=re.S)
    text = text.replace('    gtk_radio_action_set_current_value (rss->null_action, -1);', '    route_set_active (rss, NULL);')
    text = replace_block(text, '      if (rsdata->action)', '      if (rsdata->temporary)', '''      rss->syncing = TRUE;
      gtk_widget_destroy (rsdata->button);
      gtk_widget_destroy (rsdata->menu_item);
      g_object_unref (rsdata->menu_item);
      rss->syncing = FALSE;
''')
    text = text.replace('  init_radio_groups (rss);\n}', '  init_radio_groups (rss);\n  rss->active_style = NULL;\n}')
    text = text.replace('  g_object_unref (rss->accel_group);', '  g_object_unref (rss->accel_group);\n  gtk_widget_destroy (rss->null_button);\n  g_object_unref (rss->null_button);')
    return text


def transform(source, root, relative, tool):
    """Apply the appropriate reviewed selector rule to both implementations."""
    paths = {'src/hid/gtk/ghid-layer-selector.c': layer,
             'src/hid/gtk/ghid-route-style-selector.c': route}
    if relative not in paths:
        return source, 'No selector action implementation.'
    text = source.decode()
    if 'GtkActionGroup *action_group;' not in text:
        return source, 'Selector action conversion already applied.'
    return paths[relative](text).encode(), 'Direct selector widgets; targeted compile pending.'


if __name__ == '__main__':
    raise SystemExit(main('selector-actions', transform, 'python3'))
