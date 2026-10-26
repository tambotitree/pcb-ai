/* Loaded only by the native regression runner; GPL-2.0-or-later. */
#include <config.h>
#include <stdlib.h>
#include "pcb_ai_chat_internal.h"
#include "file.h"
#include "undo.h"
#include "set.h"
#include "create.h"
#include "flags.h"
#include "ghid-main-menu.h"

static Coord expected_plan_width;

/* A blank board needs the same useful main-window size as a loaded one. */
static gboolean
check_startup_size (gpointer unused G_GNUC_UNUSED)
{
  GtkWindow *window = GTK_WINDOW (gport->top_window);
  GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (window));
  GdkMonitor *monitor = gdk_display_get_monitor_at_window (display,
    gtk_widget_get_window (GTK_WIDGET (window)));
  GdkRectangle work;
  gint width, height;
  g_assert_nonnull (monitor);
  gdk_monitor_get_workarea (monitor, &work);
  gtk_window_get_size (window, &width, &height);
  g_assert_cmpint (width, >=, MIN (1280, work.width));
  g_assert_cmpint (height, >=, MIN (720, work.height));
  g_print ("PASS: usable PCB startup size %dx%d.\n", width, height);
  if (g_getenv ("PCB_AI_TEST_STARTUP_ONLY") != NULL)
    exit (0);
  return G_SOURCE_REMOVE;
}

/* Test actual foreground pixels, not merely that two PNGs differ. */
static void
check_pad_visible (PadType *pad)
{
  gint width = 100, height = 100;
  Coord x = (pad->Point1.X + pad->Point2.X) / 2;
  Coord y = (pad->Point1.Y + pad->Point2.Y) / 2;
  cairo_surface_t *surface = ghid_render_pixmap (x, y, (double) pad->Thickness / 20, width, height, 24);
  GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, width, height);
  guchar *pixels;
  gint stride, channels;
  g_assert_nonnull (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);
  stride = gdk_pixbuf_get_rowstride (pixbuf);
  channels = gdk_pixbuf_get_n_channels (pixbuf);
  /* Corner is plain substrate; center must contain visible pad copper. */
  g_assert_cmpint (memcmp (pixels + stride * 50 + channels * 50,
                          pixels + stride * 5 + channels * 5, 3), !=, 0);
  g_object_unref (pixbuf);
  cairo_surface_destroy (surface);
  /* Exercise the actual canvas presentation callback as well as offscreen HID. */
  {
    view_data saved_view = gport->view;
    cairo_t *cr;
    gport->view.coord_per_px = (double) pad->Thickness / 20;
    gport->view.width = gport->width * gport->view.coord_per_px;
    gport->view.height = gport->height * gport->view.coord_per_px;
    gport->view.x0 = x - gport->view.width / 2;
    gport->view.y0 = y - gport->view.height / 2;
    ghid_invalidate_all ();
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, gport->width, gport->height);
    cr = cairo_create (surface);
    ghid_drawing_area_expose_cb (gport->drawing_area, cr, gport);
    cairo_destroy (cr);
    pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, gport->width, gport->height);
    g_assert_nonnull (pixbuf);
    pixels = gdk_pixbuf_get_pixels (pixbuf);
    stride = gdk_pixbuf_get_rowstride (pixbuf);
    channels = gdk_pixbuf_get_n_channels (pixbuf);
    g_assert_cmpint (memcmp (pixels + stride * (gport->height / 2 + 2) + channels * (gport->width / 2 + 2),
                            pixels + stride * 5 + channels * 5, 3), !=, 0);
    g_object_unref (pixbuf);
    cairo_surface_destroy (surface);
    gport->view = saved_view;
    ghid_invalidate_all ();
  }
}

/* Verify tagged model text reaches an enabled Act button through the helper. */
static gboolean
check_tagged_patch (gpointer data)
{
  GtkWidget *panel = data;
  PcbAiChat *chat = g_object_get_data (G_OBJECT (panel), "ai-chat");
  PCBType *original = PCB;
  ElementType *element = PCB->Data->Element->data;
  PadType *pad = element->Pad->data;
  if (chat->process != NULL)
    return G_SOURCE_CONTINUE;
  g_assert_nonnull (chat->patch_content);
  g_assert_true (gtk_widget_get_sensitive (chat->apply));
  g_assert_true (TEST_FLAG (SQUAREFLAG, pad));
  g_signal_emit_by_name (chat->apply, "clicked");
  g_assert_nonnull (chat->review);
  g_assert_true (PCB == original);
  gtk_dialog_response (GTK_DIALOG (chat->review), GTK_RESPONSE_ACCEPT);
  g_assert_true (PCB != original);
  element = PCB->Data->Element->data;
  g_assert_false (TEST_FLAG (SQUAREFLAG, (PadType *) element->Pad->data));
  g_assert_cmpint (PCB->PinOn, ==, original->PinOn);
  g_assert_cmpint (PCB->ElementOn, ==, original->ElementOn);
  g_assert_cmpint (PCB->Data->Layer[0].On, ==, original->Data->Layer[0].On);
  check_pad_visible (element->Pad->data);
  Undo (true);
  g_assert_true (PCB == original);
  gtk_widget_destroy (panel);
  g_object_unref (panel);
  SetChangedFlag (false);
  g_print ("PASS: prose + tagged JSON from mock provider enables Act, review, Implement and Undo.\n");
  exit (0);
  return G_SOURCE_REMOVE;
}

/* Verify the actual async helper performs a single text-to-image continuation. */
static gboolean
check_async (gpointer data)
{
  GtkWidget *panel = data;
  PcbAiChat *chat = g_object_get_data (G_OBJECT (panel), "ai-chat");
  ElementType *element = PCB->Data->Element->data;
  PadType *pad = element->Pad->data;
  if (chat->process != NULL)
    return G_SOURCE_CONTINUE;
  g_assert_cmpstr (gtk_label_get_text (GTK_LABEL (chat->status)), ==, "Plan ready. Review it, then press Act.");
  g_assert_true (chat->image_attempted);
  g_assert_cmpfloat (chat->proposal_percent, ==, 80.0);
  g_assert_true (gtk_widget_get_sensitive (chat->apply));
  g_assert_cmpstr (gtk_button_get_label (GTK_BUTTON (chat->send)), ==, "Plan");
  g_assert_cmpstr (gtk_button_get_label (GTK_BUTTON (chat->apply)), ==, "Act");
  g_assert_cmpint (pad->Thickness, ==, expected_plan_width);
  g_signal_emit_by_name (chat->apply, "clicked");
  g_assert_nonnull (chat->review);
  g_assert_cmpint (pad->Thickness, ==, expected_plan_width);
  gtk_dialog_response (GTK_DIALOG (chat->review), GTK_RESPONSE_CANCEL);
  g_assert_null (chat->review);
  g_assert_cmpint (pad->Thickness, ==, expected_plan_width);
  g_signal_emit_by_name (chat->apply, "clicked");
  g_assert_nonnull (chat->review);
  gtk_dialog_response (GTK_DIALOG (chat->review), GTK_RESPONSE_ACCEPT);
  g_assert_cmpint (pad->Thickness, ==, expected_plan_width * 80 / 100);
  Undo (true);
  g_assert_cmpint (pad->Thickness, ==, expected_plan_width);
  g_print ("PASS: native pad/image checks and asynchronous text -> PNG -> typed proposal.\n");
  gtk_entry_set_text (GTK_ENTRY (chat->input), "Round the selected pad ends without changing its width or position");
  g_signal_emit_by_name (chat->send, "clicked");
  g_timeout_add (100, check_tagged_patch, panel);
  return G_SOURCE_REMOVE;
}

/* Exercise the second registry operation against real selected plated vias. */
static void
check_vias (PcbAiChat *chat)
{
  Coord diameter = MIL_TO_COORD (36), drill = MIL_TO_COORD (20), mask = MIL_TO_COORD (40);
  PinType *a = CreateNewVia (PCB->Data, PCB->MaxWidth / 3, PCB->MaxHeight * 3 / 4,
    diameter, MIL_TO_COORD (10), mask, drill, "", MakeFlags (SELECTEDFLAG));
  PinType *b = CreateNewVia (PCB->Data, PCB->MaxWidth / 2, PCB->MaxHeight * 3 / 4,
    diameter, MIL_TO_COORD (10), mask, drill, "", MakeFlags (SELECTEDFLAG));
  PinType *other = CreateNewVia (PCB->Data, PCB->MaxWidth * 2 / 3, PCB->MaxHeight * 3 / 4,
    diameter, MIL_TO_COORD (10), mask, drill, "", NoFlags ());
  g_assert_nonnull (a);
  g_assert_nonnull (b);
  g_assert_nonnull (other);
  g_free (chat->request_board);
  g_free (chat->selection);
  chat->request_board = pcb_board_snapshot ();
  chat->selection = pcb_ai_selection_manifest ();
  g_assert_nonnull (strstr (chat->selection, "via id="));
  chat->proposal_kind = PCB_AI_EDIT_VIA_DIAMETER;
  chat->proposal_percent = 120;
  pcb_ai_apply_clicked (NULL, chat);
  g_assert_cmpint (a->Thickness, ==, diameter * 120 / 100);
  g_assert_cmpint (b->Thickness, ==, diameter * 120 / 100);
  g_assert_cmpint (other->Thickness, ==, diameter);
  g_assert_cmpint (a->DrillingHole, ==, drill);
  g_assert_cmpint (a->X, ==, PCB->MaxWidth / 3);
  g_assert_cmpint (a->Y, ==, PCB->MaxHeight * 3 / 4);
  g_assert_cmpint (a->BuriedFrom, ==, 0);
  g_assert_cmpint (a->BuriedTo, ==, 0);
  g_assert_cmpint (a->Mask, ==, mask + diameter * 20 / 100);
  Undo (true);
  g_assert_cmpint (a->Thickness, ==, diameter);
  g_assert_cmpint (b->Thickness, ==, diameter);
  g_assert_cmpint (a->Mask, ==, mask);
  /* Reject the entire set when any selected via is locked. */
  SET_FLAG (LOCKFLAG, b);
  g_free (chat->request_board);
  g_free (chat->selection);
  chat->request_board = pcb_board_snapshot ();
  chat->selection = pcb_ai_selection_manifest ();
  chat->proposal_kind = PCB_AI_EDIT_VIA_DIAMETER;
  chat->proposal_percent = 120;
  pcb_ai_apply_clicked (NULL, chat);
  g_assert_cmpint (a->Thickness, ==, diameter);
  g_assert_cmpint (b->Thickness, ==, diameter);
  CLEAR_FLAG (SELECTEDFLAG, a);
  CLEAR_FLAG (SELECTEDFLAG, b);
  g_print ("PASS: selected via enlargement, unchanged drill/position/layers, mask/undo, locked-set rejection.\n");
}

/* Exercise the real parser, popup gate, board transaction and native Undo/Redo. */
static void
check_patch_review (PcbAiChat *chat)
{
  PCBType *original = PCB;
  ElementType *element = PCB->Data->Element->data;
  PadType *pad = element->Pad->data;
  gchar *snapshot = pcb_board_snapshot ();
  gchar *square = strstr (snapshot, "square");
  GString *candidate = g_string_new (snapshot);
  gchar *content64, *diff64, *text64, *envelope, *reply = NULL;
  gchar *before, *after;
  gchar **lines;
  GString *diff = g_string_new ("--- a/current.pcb\n+++ b/current.pcb\n");
  gboolean image = FALSE;
  guint line;
  g_assert_nonnull (square);
  g_assert_true (TEST_FLAG (SQUAREFLAG, pad));
  g_string_erase (candidate, square - snapshot, strlen ("square"));
  lines = g_strsplit (snapshot, "\n", -1);
  for (line = 0; lines[line] != NULL; line++)
    if (strstr (lines[line], "square") != NULL)
      {
        gchar *changed = g_strdup (lines[line]);
        gchar *flag = strstr (changed, "square");
        memmove (flag, flag + strlen ("square"), strlen (flag + strlen ("square")) + 1);
        g_string_append_printf (diff, "@@ -%u +%u @@\n-%s\n+%s\n", line + 1, line + 1, lines[line], changed);
        g_free (changed);
        break;
      }
  g_strfreev (lines);
  g_free (chat->request_board);
  g_free (chat->selection);
  chat->request_board = g_strdup (snapshot);
  chat->selection = pcb_ai_selection_manifest ();
  chat->page_identity = PCB;
  content64 = g_base64_encode ((const guchar *) candidate->str, candidate->len);
  diff64 = g_base64_encode ((const guchar *) diff->str, diff->len);
  text64 = g_base64_encode ((const guchar *) "Round the first pad", strlen ("Round the first pad"));
  envelope = g_strdup_printf ("[response]\nkind=reply\ntext=%s\npatch_count=1\npatch_0_path=Y3VycmVudC5wY2I=\npatch_0_content=%s\npatch_0_diff=%s\n", text64, content64, diff64);
  g_assert_true (pcb_ai_read_response (chat, envelope, &reply, &image));
  g_assert_nonnull (chat->patch_content);
  before = pcb_ai_viewport_png (chat);
  pcb_ai_review_clicked (NULL, chat);
  g_assert_nonnull (chat->review);
  g_assert_true (PCB == original);
  g_assert_true (TEST_FLAG (SQUAREFLAG, pad));
  gtk_dialog_response (GTK_DIALOG (chat->review), GTK_RESPONSE_CANCEL);
  g_assert_true (PCB == original);
  pcb_ai_review_clicked (NULL, chat);
  gtk_dialog_response (GTK_DIALOG (chat->review), GTK_RESPONSE_ACCEPT);
  g_assert_true (PCB != original);
  element = PCB->Data->Element->data;
  g_assert_false (TEST_FLAG (SQUAREFLAG, (PadType *) element->Pad->data));
  g_assert_cmpint (PCB->PinOn, ==, original->PinOn);
  g_assert_cmpint (PCB->ElementOn, ==, original->ElementOn);
  g_assert_cmpint (PCB->Data->Layer[0].On, ==, original->Data->Layer[0].On);
  check_pad_visible (element->Pad->data);
  /* Restore the same viewport after the board-changed fit for pixel comparison. */
  gport->view.coord_per_px = (gdouble) pad->Thickness / 20.0;
  gport->view.width = gport->width * gport->view.coord_per_px;
  gport->view.height = gport->height * gport->view.coord_per_px;
  gport->view.x0 = pad->Point1.X - gport->view.width / 2;
  gport->view.y0 = pad->Point1.Y - gport->view.height / 2;
  after = pcb_ai_viewport_png (chat);
  g_assert_cmpstr (before, !=, after);
  Undo (true);
  g_assert_true (PCB == original);
  g_assert_true (TEST_FLAG (SQUAREFLAG, pad));
  Redo (true);
  g_assert_true (PCB != original);
  Undo (true);
  g_assert_true (PCB == original);
  /* An invalid native design must never enable Implement. */
  chat->patch_content = g_strdup ("not a PCB\n");
  chat->patch_diff = g_strdup ("invalid test patch");
  pcb_ai_review_clicked (NULL, chat);
  g_assert_nonnull (chat->review);
  g_assert_false (gtk_widget_get_sensitive (gtk_dialog_get_widget_for_response (GTK_DIALOG (chat->review), GTK_RESPONSE_ACCEPT)));
  gtk_dialog_response (GTK_DIALOG (chat->review), GTK_RESPONSE_CANCEL);
  pcb_ai_patch_clear (chat);
  g_assert_true (PCB == original);
  g_free (before); g_free (after); g_free (snapshot); g_free (reply);
  g_free (content64); g_free (diff64); g_free (text64); g_free (envelope);
  g_string_free (candidate, TRUE); g_string_free (diff, TRUE);
  g_print ("PASS: patch review/cancel, native parser, changed image, Implement, whole-board Undo/Redo, invalid-patch rejection.\n");
}

/* Locate a real menu endpoint, traversing submenus without relying on indices. */
static GtkWidget *
find_ai_menu (GtkWidget *widget)
{
  GList *children, *item;
  GtkWidget *found = NULL;
  if (GTK_IS_MENU_ITEM (widget))
    {
      GtkWidget *submenu;
      if (g_strcmp0 (gtk_menu_item_get_label (GTK_MENU_ITEM (widget)), "AI Assist...") == 0)
        return widget;
      submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (widget));
      if (submenu != NULL)
        return find_ai_menu (submenu);
    }
  if (!GTK_IS_CONTAINER (widget))
    return NULL;
  children = gtk_container_get_children (GTK_CONTAINER (widget));
  for (item = children; item != NULL && found == NULL; item = item->next)
    found = find_ai_menu (item->data);
  g_list_free (children);
  return found;
}

/* Regression: actual activation plus cached-item removal must remain safe. */
static void
check_menu_lifetime (void)
{
  GtkWidget *item = find_ai_menu (ghidgui->menu_bar);
  GtkWidget *dialog;
  g_assert_nonnull (item);
  gtk_menu_item_activate (GTK_MENU_ITEM (item));
  dialog = g_object_get_data (G_OBJECT (gport->top_window), "pcb-ai-dialog");
  g_assert_nonnull (dialog);
  gtk_widget_destroy (dialog);
  gtk_widget_destroy (item);
  ghid_update_toggle_flags ();
  g_print ("PASS: menu activation and toggle sync after item destruction.\n");
}

/* Exercise real core resize/undo and the Cairo viewport in an isolated process. */
static gboolean
check_native (gpointer unused G_GNUC_UNUSED)
{
  ElementType *element = PCB->Data->Element->data;
  PadType *first = element->Pad->data;
  PadType *second = element->Pad->next->data;
  ElementType *other = PCB->Data->Element->next->data;
  PadType *untouched = other->Pad->data;
  Coord width = first->Thickness, mask = first->Mask;
  Coord other_width = untouched->Thickness;
  GtkWidget *host = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *panel = pcb_ai_chat_new (GTK_WINDOW (gport->top_window));
  PcbAiChat *chat;
  gchar *before, *after;
  check_startup_size (NULL);
  check_menu_lifetime ();
  g_object_ref_sink (panel);
  gtk_container_add (GTK_CONTAINER (host), panel);
  chat = g_object_get_data (G_OBJECT (panel), "ai-chat");
  SET_FLAG (SELECTEDFLAG, first);
  SET_FLAG (SELECTEDFLAG, second);
  chat->page_identity = PCB;
  chat->request_board = pcb_board_snapshot ();
  chat->selection = pcb_ai_selection_manifest ();
  chat->proposal_percent = 80;
  chat->proposal_kind = PCB_AI_EDIT_PAD_WIDTH;
  gport->view.coord_per_px = (gdouble) width / 20.0;
  gport->view.width = gport->width * gport->view.coord_per_px;
  gport->view.height = gport->height * gport->view.coord_per_px;
  gport->view.x0 = first->Point1.X - gport->view.width / 2;
  gport->view.y0 = first->Point1.Y - gport->view.height / 2;
  before = pcb_ai_viewport_png (chat);
  g_assert_nonnull (before);
  g_assert_nonnull (strstr (chat->request_board, "geometry omitted"));
  g_assert_null (strstr (chat->request_board, "SymbolLine["));
  pcb_ai_apply_clicked (NULL, chat);
  g_assert_cmpint (first->Thickness, ==, width * 80 / 100);
  g_assert_cmpint (second->Thickness, ==, width * 80 / 100);
  g_assert_cmpint (first->Mask, ==, mask - width * 20 / 100);
  g_assert_cmpint (untouched->Thickness, ==, other_width);
  after = pcb_ai_viewport_png (chat);
  g_assert_nonnull (after);
  g_assert_true (strcmp (before, after) != 0);
  Undo (true);
  g_assert_cmpint (first->Thickness, ==, width);
  g_assert_cmpint (second->Thickness, ==, width);
  g_assert_cmpint (first->Mask, ==, mask);
  /* A changed selection invalidates the old proposal. */
  CLEAR_FLAG (SELECTEDFLAG, second);
  chat->proposal_percent = 80;
  chat->proposal_kind = PCB_AI_EDIT_PAD_WIDTH;
  pcb_ai_apply_clicked (NULL, chat);
  g_assert_cmpint (first->Thickness, ==, width);
  g_assert_nonnull (strstr (gtk_label_get_text (GTK_LABEL (chat->status)), "Cannot apply"));
  g_free (before);
  g_free (after);
  check_patch_review (chat);
  check_vias (chat);
  expected_plan_width = first->Thickness;
  gtk_combo_box_set_active_id (GTK_COMBO_BOX (chat->provider), "compatible");
  gtk_entry_set_text (GTK_ENTRY (chat->endpoint), g_getenv ("PCB_AI_TEST_ENDPOINT"));
  gtk_entry_set_text (GTK_ENTRY (chat->key), "fixture-key");
  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (chat->models))), "fixture-model");
  gtk_entry_set_text (GTK_ENTRY (chat->input), "Reduce selected pad width by 20 percent");
  g_signal_emit_by_name (chat->send, "clicked");
  g_timeout_add (100, check_async, panel);
  return G_SOURCE_REMOVE;
}

/* Let GTK finish constructing the main canvas before invoking the checks. */
static int
run_test (int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED,
          Coord x G_GNUC_UNUSED, Coord y G_GNUC_UNUSED)
{
  g_timeout_add (500, g_getenv ("PCB_AI_TEST_STARTUP_ONLY") != NULL ? check_startup_size : check_native, NULL);
  return 0;
}

/* The runner loads this private plugin solely in its temporary working dir. */
void
pcb_plugin_init (void)
{
  static HID_Action action[] = {
    {.name = "AiVisualRegression", .trigger_cb = run_test,
     .description = "Private regression fixture", .syntax = "AiVisualRegression()"}
  };
  hid_register_actions (action, G_N_ELEMENTS (action));
}
