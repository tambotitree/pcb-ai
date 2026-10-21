#!/usr/bin/env python3
"""Convert resource main-menu actions to typed, direct GTK menu-item callbacks."""

from conversion import main


def replace_block(text, start, end, replacement):
    """Replace a reviewed block delimited by unique source markers."""
    a = text.index(start)
    b = text.index(end, a)
    return text[:a] + replacement + text[b:]


def transform(source, root, relative, tool):
    """Convert the main-menu contract and its caller together; leave selectors separate."""
    paths = ('src/hid/gtk/ghid-main-menu.c', 'src/hid/gtk/ghid-main-menu.h',
             'src/hid/gtk/gui-top-window.c')
    if relative not in paths:
        return source, 'No main-menu contract in this file.'
    text = source.decode()
    if relative.endswith('ghid-main-menu.c'):
        if 'GtkActionGroup *action_group;' not in text:
            return source, 'Main-menu conversion already applied.'
        text = text.replace('static int action_counter;\n', '')
        text = text.replace('  GtkActionGroup *action_group;\n', '')
        text = text.replace('  GCallback action_cb;', '  GHidMenuActivateFunc action_cb;')
        text = text.replace('GtkAction *', 'GtkWidget *')
        text = text.replace('ghid_main_menu_new (GCallback action_cb,',
                            'ghid_main_menu_new (GHidMenuActivateFunc action_cb,')
        text = replace_block(text, '                  gchar *name = g_strdup_printf',
                             '                }\n              else if', '''                  action = gtk_check_menu_item_new_with_mnemonic (menu_label);
                  gtk_check_menu_item_set_draw_as_radio
                    (GTK_CHECK_MENU_ITEM (action), !!strchr (checked, ','));
''')
        text = replace_block(text, '                  gchar *name = g_strdup_printf',
                             '                }\n            }', '''                  action = gtk_menu_item_new_with_mnemonic (menu_label);
''')
        text = replace_block(text, '              GtkWidget *item;\n              gtk_action_set_accel_group',
                             '              menu->special_key_cb', '''              guint key;
              GdkModifierType modifiers;
              gulong handler;
              gtk_widget_set_tooltip_text (action, resource_value (sub_res, "tip"));
              if (accel && *accel)
                {
                  gtk_accelerator_parse (accel, &key, &modifiers);
                  if (key)
                    gtk_widget_add_accelerator (action, "activate", menu->accel_group,
                                                key, modifiers, GTK_ACCEL_VISIBLE);
                }
              handler = g_signal_connect (action, "activate",
                                           G_CALLBACK (menu_activate_cb), menu);
              g_object_set_data_full (G_OBJECT (action), "activate-handler",
                                      g_memdup2 (&handler, sizeof handler), g_free);
              g_object_set_data (G_OBJECT (action), "resource", (gpointer) sub_res);
              gtk_menu_shell_append (shell, action);
              menu->actions = g_list_append (menu->actions, action);
''')
        text = text.replace('              const char *tip = resource_value (sub_res, "tip");\n','')
        text = text.replace('  mm->action_group = gtk_action_group_new ("MainMenu");\n','')
        text = text.replace('  /* Hookup signal handlers */', '  (void) mm;')
        text = text.replace('ghid_main_menu_class_init (GHidMainMenuClass *klass)\n{',
                            'ghid_main_menu_class_init (GHidMainMenuClass *klass)\n{\n  (void) klass;')
        text = replace_block(text, '      Resource *res = g_object_get_data',
                             '    }\n}', '''      GtkWidget *item = GTK_WIDGET (list->data);
      const gulong *handler = g_object_get_data (G_OBJECT (item), "activate-handler");
      const char *tf = g_object_get_data (G_OBJECT (item), "checked-flag");
      const char *af = g_object_get_data (G_OBJECT (item), "active-flag");
      if (handler) g_signal_handler_block (item, *handler);
      cb (item, tf, af);
      if (handler) g_signal_handler_unblock (item, *handler);
''')
        text = text.replace('/* SIGNAL HANDLERS */', '''/* SIGNAL HANDLERS */
/*! \\brief Dispatch a menu activation through the typed resource callback. */
static void
menu_activate_cb (GtkWidget *item, GHidMainMenu *menu)
{
  const Resource *node = g_object_get_data (G_OBJECT (item), "resource");
  menu->action_cb (item, node);
}
''')
    elif relative.endswith('ghid-main-menu.h'):
        if 'typedef void (*GHidMenuActivateFunc)' in text:
            return source, 'Main-menu contract already converted.'
        text = text.replace('GType ghid_main_menu_get_type (void);',
            'typedef void (*GHidMenuActivateFunc) (GtkWidget *, const Resource *);\n\nGType ghid_main_menu_get_type (void);')
        text = text.replace('GCallback action_cb', 'GHidMenuActivateFunc action_cb')
        text = text.replace('GtkAction *', 'GtkWidget *')
    else:
        text = text.replace('GtkAction *', 'GtkWidget *')
        text = text.replace('gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (act)',
                            'gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (act)')
        text = text.replace('gtk_action_set_sensitive (act', 'gtk_widget_set_sensitive (act')
        text = text.replace('ghid_main_menu_new (G_CALLBACK (ghid_menu_cb),',
                            'ghid_main_menu_new (ghid_menu_cb,')
    return text.encode(), 'Main-menu widgets and typed callback contract converted; compile pending.'


if __name__ == '__main__':
    raise SystemExit(main('menu-actions', transform, 'python3'))
