/*!
 * \file src/hid/gtk/ghid-main-menu.c
 *
 * \brief Implementation of GHidMainMenu widget.
 *
 * This widget is the main pcb menu.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996, 2004 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact addresses for paper mail and Email:
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 * Thomas.Nau@rz.uni-ulm.de
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gtkhid.h"
#include "gui.h"
#include "pcb-printf.h"

#include "ghid-main-menu.h"
#include "ghid-layer-selector.h"
#include "ghid-route-style-selector.h"

void Message (const char *, ...);


struct _GHidMainMenu
{
  GtkMenuBar parent;

  GtkAccelGroup *accel_group;

  gint layer_view_pos;
  gint layer_pick_pos;
  gint route_style_pos;

  GtkMenuShell *layer_view_shell;
  GtkMenuShell *layer_pick_shell;
  GtkMenuShell *route_style_shell;

  GList *actions;
  GHashTable *popup_table;

  gint n_layer_views;
  gint n_layer_picks;
  gint n_route_styles;

  GHidMenuActivateFunc action_cb;
  void (*special_key_cb) (const char *accel, GtkWidget *action,
                          const Resource *node);
};

struct _GHidMainMenuClass
{
  GtkMenuBarClass parent_class;
};

/* TODO: write finalize function */

/* SIGNAL HANDLERS */
/*! \brief Dispatch a menu activation through the typed resource callback. */
static void
menu_activate_cb (GtkWidget *item, GHidMainMenu *menu)
{
  const Resource *node = g_object_get_data (G_OBJECT (item), "resource");
  menu->action_cb (item, node);
}


/* A cached menu item is a borrowed widget: remove it when GTK destroys it. */
static void
menu_item_destroyed_cb (GtkWidget *item, GHidMainMenu *menu)
{
  menu->actions = g_list_remove (menu->actions, item);
}

/* RESOURCE HANDLER */

/*!
 * \brief Translate gpcb-menu.res accelerators to gtk ones.
 *
 * Some keys need to be replaced by a name for the gtk accelerators to
 * work.  This table contains the translations.  The "in" character is
 * what would appear in gpcb-menu.res and the "out" string is what we
 * have to feed to gtk.  I was able to find these by using xev to find
 * the keycode and then looked at gtk+-2.10.9/gdk/keynames.txt (from the
 * gtk source distribution) to figure out the names that go with the
 * codes.
 */
static gchar *
translate_accelerator (const char *text)
{
  GString *ret_val = g_string_new ("");
  static struct { const char *in, *out; } key_table[] =
  {
    {"Enter", "Return"},
    {"Alt",   "<alt>"},
    {"Shift", "<shift>"},
    {"Ctrl",  "<ctrl>"},
    {" ", ""},
    {":", "colon"},
    {"=", "equal"},
    {"/", "slash"},
    {"[", "bracketleft"},
    {"]", "bracketright"},
    {".", "period"},
    {"|", "bar"},
    {"+", "plus"},
    {"-", "minus"},
    {NULL, NULL}
  };

  enum {MOD, KEY} state = MOD;
  while (*text != '\0')
    {
      static gboolean gave_msg;
      gboolean found = FALSE;
      int i;

      if (state == MOD && strncmp (text, "<Key>", 5) == 0)
        {
          state = KEY;
          text += 5;
        }
      for (i = 0; key_table[i].in != NULL; ++i)
        {
          int len = strlen (key_table[i].in);
          if (strncmp (text, key_table[i].in, len) == 0)
            {
              found = TRUE;
              g_string_append (ret_val, key_table[i].out);
              text += len;
            }
        }
      if (found == FALSE)
        switch (state)
          {
          case MOD:
            Message (_("Don't know how to parse \"%s\" as an "
                       "accelerator in the menu resource file.\n"),
                     text);
            if (!gave_msg)
              {
                gave_msg = TRUE;
                Message (_("Format is:\n"
                           "modifiers<Key>k\n"
                           "where \"modifiers\" is a space "
                           "separated list of key modifiers\n"
                           "and \"k\" is the name of the key.\n"
                           "Allowed modifiers are:\n"
                           "   Ctrl\n"
                           "   Shift\n"
                           "   Alt\n"
                           "Please note that case is important.\n"));
              }
            break;
          case KEY:
            g_string_append_c (ret_val, *text);
            ++text;
            break;
          }
    }
  return g_string_free (ret_val, FALSE);
}

static gboolean
g_str_case_equal (gconstpointer v1, gconstpointer v2)
{
  return strcasecmp (v1, v2);
}

/*!
 * \brief Check that translated accelerators are unique; warn otherwise.
 */
static const char *
check_unique_accel (const char *accelerator)
{
  static GHashTable *accel_table;

  if (!accelerator ||*accelerator)
    return accelerator;

  if (!accel_table)
    accel_table = g_hash_table_new (g_str_hash, g_str_case_equal);

  if (g_hash_table_lookup (accel_table, accelerator))
    {
       Message (_("Duplicate accelerator found: \"%s\"\n"
                  "The second occurrence will be dropped\n"),
                accelerator);
        return NULL;
    }

  g_hash_table_insert (accel_table,
                       (gpointer) accelerator, (gpointer) accelerator);

  return accelerator;
}


/*!
 * \brief Translate a resource tree into a menu structure.
 *
 * \param [in] menu    The GHidMainMenu widget to be acted on.
 * \param [in] shall   The base menu shell (a menu bar or popup menu).
 * \param [in] res     The base of the resource tree.
 * */
void
ghid_main_menu_real_add_resource (GHidMainMenu *menu, GtkMenuShell *shell,
                                  const Resource *res)
{
  int i, j;
  const Resource *tmp_res;
  gchar mnemonic = 0;

  for (i = 0; i < res->c; ++i)
    {
      const gchar *accel = NULL;
      char *menu_label;
      const char *res_val;
      const Resource *sub_res = res->v[i].subres;
      GtkWidget *action = NULL;

      switch (resource_type (res->v[i]))
        {
        case 101:   /* name, subres: passthrough */
          ghid_main_menu_real_add_resource (menu, shell, sub_res);
          break;
        case   1:   /* no name, subres */
          tmp_res = resource_subres (sub_res, "a");  /* accelerator */
          res_val = resource_value (sub_res, "m");   /* mnemonic */
          if (res_val)
            mnemonic = res_val[0];
          /* The accelerator resource will have two values, like
           *   a={"Ctrl-Q" "Ctrl<Key>q"}
           * The first Gtk ignores. The second needs to be translated. */
          if (tmp_res)
            accel = check_unique_accel
                      (translate_accelerator (tmp_res->v[1].value));

          /* Now look for the first unnamed value (not a subresource) to
           * figure out the name of the menu or the menuitem. */
          res_val = "button";
          for (j = 0; j < sub_res->c; ++j)
            if (resource_type (sub_res->v[j]) == 10)
              {
                res_val = _(sub_res->v[j].value);
                break;
              }
          /* Hack '_' in based on mnemonic value */
          if (!mnemonic)
            menu_label = g_strdup (res_val);
          else
            {
              char *post_ = strchr (res_val, mnemonic);
              if (post_ == NULL)
                menu_label = g_strdup (res_val);
              else
                {
                  GString *tmp = g_string_new ("");
                  g_string_append_len (tmp, res_val, post_ - res_val);
                  g_string_append_c (tmp, '_');
                  g_string_append (tmp, post_);
                  menu_label = g_string_free (tmp, FALSE);
                }
            }
          /* If the subresource we're processing also has unnamed
           * subresources, it's a submenu, not a regular menuitem. */
          if (sub_res->flags & FLAG_S)
            {
              /* SUBMENU */
              GtkWidget *submenu = gtk_menu_new ();
              GtkWidget *item = gtk_menu_item_new_with_mnemonic (menu_label);

              gtk_menu_shell_append (shell, item);
              gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

              /* recurse on the newly-added submenu */
              ghid_main_menu_real_add_resource (menu,
                                                GTK_MENU_SHELL (submenu),
                                                sub_res);
            }
          else
            {
              /* NON-SUBMENU: MENU ITEM */
              const char *checked = resource_value (sub_res, "checked");
              const char *label = resource_value (sub_res, "sensitive");
              if (checked)
                {
                  /* TOGGLE ITEM */
                  action = gtk_check_menu_item_new_with_mnemonic (menu_label);
                  gtk_check_menu_item_set_draw_as_radio
                    (GTK_CHECK_MENU_ITEM (action), !!strchr (checked, ','));
                }
              else if (label && strcmp (label, "false") == 0)
                {
                  /* INSENSITIVE ITEM */
                  GtkWidget *item = gtk_menu_item_new_with_label (menu_label);
                  gtk_widget_set_sensitive (item, FALSE);
                  gtk_menu_shell_append (shell, item);
                }
              else
                {
                  /* NORMAL ITEM */
                  action = gtk_menu_item_new_with_mnemonic (menu_label);
                }
            }
          /* Connect accelerator, if there is one */
          if (action)
            {
              guint key;
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
              g_signal_connect_object (action, "destroy", G_CALLBACK (menu_item_destroyed_cb), menu, 0);
              menu->special_key_cb (accel, action, sub_res);
            }
          /* Scan rest of resource in case there is more work */
          for (j = 0; j < sub_res->c; j++)
            {
              const char *res_name;
              /* named value = X resource */
              if (resource_type (sub_res->v[j]) == 110)
                {
                  res_name = sub_res->v[j].name;

                  /* translate bg, fg to background, foreground */
                  if (strcmp (res_name, "fg") == 0)   res_name = "foreground";
                  if (strcmp (res_name, "bg") == 0)   res_name = "background";

                  /* ignore special named values (m, a, sensitive) */
                  if (strcmp (res_name, "m") == 0
                      || strcmp (res_name, "a") == 0
                      || strcmp (res_name, "sensitive") == 0
                      || strcmp (res_name, "tip") == 0)
                    break;

                  /* log checked and active special values */
                  if (action && strcmp (res_name, "checked") == 0)
                    g_object_set_data (G_OBJECT (action), "checked-flag",
                                       sub_res->v[j].value);
                  else if (action && strcmp (res_name, "active") == 0)
                    g_object_set_data (G_OBJECT (action), "active-flag",
                                       sub_res->v[j].value);
                  else
                    /* if we got this far it is supposed to be an X
                     * resource.  For now ignore it and warn the user */
                    Message (_("The gtk gui currently ignores \"%s\""
                               "as part of a menuitem resource.\n"
                               "Feel free to provide patches\n"),
                             sub_res->v[j].value);
                }
            }
          break;
        case  10:   /* no name, value */
          /* If we get here, the resource is "-" or "@foo" for some foo */
          if (res->v[i].value[0] == '@')
            {
              GList *children;
              int pos;

              children = gtk_container_get_children (GTK_CONTAINER (shell));
              pos = g_list_length (children);
              g_list_free (children);

              if (strcmp (res->v[i].value, "@layerview") == 0)
                {
                  menu->layer_view_shell = shell;
                  menu->layer_view_pos = pos;
                }
              else if (strcmp (res->v[i].value, "@layerpick") == 0)
                {
                  menu->layer_pick_shell = shell;
                  menu->layer_pick_pos = pos;
                }
              else if (strcmp (res->v[i].value, "@routestyles") == 0)
                {
                  menu->route_style_shell = shell;
                  menu->route_style_pos = pos;
                }
              else
                Message (_("GTK GUI currently ignores \"%s\" in the menu\n"
                           "resource file.\n"), res->v[i].value);
            }
          else if (strcmp (res->v[i].value, "-") == 0)
            {
              GtkWidget *item = gtk_separator_menu_item_new ();
              gtk_menu_shell_append (shell, item);
            }
          else if (i > 0)
            {
              /* This is an action-less menuitem. It is really only useful
               * when you're starting to build a new menu and you're looking
               * to get the layout right. */
              GtkWidget *item
                = gtk_menu_item_new_with_label (_(res->v[i].value));
              gtk_menu_shell_append (shell, item);
            }
          break;
      }
  }
}

/* CONSTRUCTOR */
static void
ghid_main_menu_init (GHidMainMenu *mm)
{
  (void) mm;
}

static void
ghid_main_menu_class_init (GHidMainMenuClass *klass)
{
  (void) klass;
}

/* PUBLIC FUNCTIONS */
/*! \brief Adapt GLib class initialization to the typed class. */
static void
ghid_main_menu_class_init_adapter (gpointer klass, gpointer data)
{
  (void) data;
  ghid_main_menu_class_init ((GHidMainMenuClass *) klass);
}

/*! \brief Adapt GLib instance initialization to the typed widget. */
static void
ghid_main_menu_init_adapter (GTypeInstance *instance, gpointer klass)
{
  (void) klass;
  ghid_main_menu_init ((GHidMainMenu *) instance);
}

GType
ghid_main_menu_get_type (void)
{
  static GType mm_type = 0;

  if (!mm_type)
    {
      const GTypeInfo mm_info =
        {
          sizeof (GHidMainMenuClass),
          NULL, /* base_init */
          NULL, /* base_finalize */
          ghid_main_menu_class_init_adapter,
          NULL, /* class_finalize */
          NULL, /* class_data */
          sizeof (GHidMainMenu),
          0,    /* n_preallocs */
          ghid_main_menu_init_adapter,
        NULL /* value_table */
        };

      mm_type = g_type_register_static (GTK_TYPE_MENU_BAR,
                                        "GHidMainMenu",
                                        &mm_info, 0);
    }

  return mm_type;
}

/*!
 * \brief Create a new GHidMainMenu.
 *
 * \return a freshly-allocated GHidMainMenu
 */
GtkWidget *
ghid_main_menu_new (GHidMenuActivateFunc action_cb,
                    void (*special_key_cb) (const char *accel,
                                            GtkWidget *action,
                                            const Resource *node))
{
  GHidMainMenu *mm = g_object_new (GHID_MAIN_MENU_TYPE, NULL);

  mm->accel_group = gtk_accel_group_new ();

  mm->layer_view_pos = 0;
  mm->layer_pick_pos = 0;
  mm->route_style_pos = 0;
  mm->n_layer_views = 0;
  mm->n_layer_picks = 0;
  mm->n_route_styles = 0;
  mm->layer_view_shell = NULL;
  mm->layer_pick_shell = NULL;
  mm->route_style_shell = NULL;

  mm->special_key_cb = special_key_cb;
  mm->action_cb = action_cb;
  mm->actions = NULL;
  mm->popup_table = g_hash_table_new (g_str_hash, g_str_equal);

  return GTK_WIDGET (mm);
}

/*!
 * \brief Turn a pcb resource into the main menu.
 */
void
ghid_main_menu_add_resource (GHidMainMenu *menu, const Resource *res)
{
  ghid_main_menu_real_add_resource (menu, GTK_MENU_SHELL (menu), res);
}

/*!
 * \brief Turn a pcb resource into a popup menu.
 */
void
ghid_main_menu_add_popup_resource (GHidMainMenu *menu, const char *name,
                                   const Resource *res)
{
  GtkWidget *new_menu = gtk_menu_new ();
  g_object_ref_sink (new_menu);
  ghid_main_menu_real_add_resource (menu, GTK_MENU_SHELL (new_menu), res);
  g_hash_table_insert (menu->popup_table, (gpointer) name, new_menu);
  gtk_widget_show_all (new_menu);
}

/*!
 * \brief Returns a registered popup menu by name.
 */
GtkMenu *
ghid_main_menu_get_popup (GHidMainMenu *menu, const char *name)
{
  return g_hash_table_lookup (menu->popup_table, name);
}


/*!
 * \brief Updates the toggle/active state of all items.
 *
 * Loops through all actions, passing the action, its toggle
 * flag (maybe NULL), and its active flag (maybe NULL), to a
 * callback function. It is the responsibility of the function
 * to actually change the state of the action.
 *
 * \param [in] menu    The menu to be acted on.
 * \param [in] cb      The callback that toggles the actions.
 */
void
ghid_main_menu_update_toggle_state (GHidMainMenu *menu,
                                    void (*cb) (GtkWidget *,
                                                const char *toggle_flag,
                                                const char *active_flag))
{
  GList *list;
  for (list = menu->actions; list; list = list->next)
    {
      GtkWidget *item = GTK_WIDGET (list->data);
      const gulong *handler = g_object_get_data (G_OBJECT (item), "activate-handler");
      const char *tf = g_object_get_data (G_OBJECT (item), "checked-flag");
      const char *af = g_object_get_data (G_OBJECT (item), "active-flag");
      if (handler) g_signal_handler_block (item, *handler);
      cb (item, tf, af);
      if (handler) g_signal_handler_unblock (item, *handler);
    }
}

/*!
 * \brief Installs or updates layer selector items.
 */
void
ghid_main_menu_install_layer_selector (GHidMainMenu *mm,
                                       GHidLayerSelector *ls)
{
  GList *children, *iter;

  /* @layerview */
  if (mm->layer_view_shell)
    {
      /* Remove old children */
      children = gtk_container_get_children
                   (GTK_CONTAINER (mm->layer_view_shell));
      for (iter = g_list_nth (children, mm->layer_view_pos);
           iter != NULL && mm->n_layer_views > 0;
           iter = g_list_next (iter), mm->n_layer_views --)
        gtk_container_remove (GTK_CONTAINER (mm->layer_view_shell),
                              iter->data);
      g_list_free (children);

      /* Install new ones */
      mm->n_layer_views = ghid_layer_selector_install_view_items
                            (ls, mm->layer_view_shell, mm->layer_view_pos);
    }

  /* @layerpick */
  if (mm->layer_pick_shell)
    {
      /* Remove old children */
      children = gtk_container_get_children
                   (GTK_CONTAINER (mm->layer_pick_shell));
      for (iter = g_list_nth (children, mm->layer_pick_pos);
           iter != NULL && mm->n_layer_picks > 0;
           iter = g_list_next (iter), mm->n_layer_picks --)
        gtk_container_remove (GTK_CONTAINER (mm->layer_pick_shell),
                              iter->data);
      g_list_free (children);

      /* Install new ones */
      mm->n_layer_picks = ghid_layer_selector_install_pick_items
                            (ls, mm->layer_pick_shell, mm->layer_pick_pos);
    }
}

/*!
 * \brief Installs or updates route style selector items.
 */
void
ghid_main_menu_install_route_style_selector (GHidMainMenu *mm,
                                             GHidRouteStyleSelector *rss)
{
  GList *children, *iter;
  /* @routestyles */
  if (mm->route_style_shell)
    {
      /* Remove old children */
      children = gtk_container_get_children
                   (GTK_CONTAINER (mm->route_style_shell));
      for (iter = g_list_nth (children, mm->route_style_pos);
           iter != NULL && mm->n_route_styles > 0;
           iter = g_list_next (iter), mm->n_route_styles --)
        gtk_container_remove (GTK_CONTAINER (mm->route_style_shell),
                              iter->data);
      g_list_free (children);
      /* Install new ones */
      mm->n_route_styles = ghid_route_style_selector_install_items
                             (rss, mm->route_style_shell, mm->route_style_pos);
    }
}

/*!
 * \brief Returns the menu bar's accelerator group.
 */
GtkAccelGroup *
ghid_main_menu_get_accel_group (GHidMainMenu *menu)
{
  return menu->accel_group;
}
