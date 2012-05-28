/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 *
 * Copyright (C) 2010 Intel, Inc
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Thomas Wood <thomas.wood@intel.com>
 *          Rodrigo Moya <rodrigo@gnome.org>
 *
 */

#include <config.h>

#include <math.h>
#include <glib/gi18n-lib.h>
#include <gdesktop-enums.h>
#include "cc-ua-panel.h"

#include "zoom-options.h"

#define WID(b, w) (GtkWidget *) gtk_builder_get_object (b, w)


G_DEFINE_DYNAMIC_TYPE (CcUaPanel, cc_ua_panel, CC_TYPE_PANEL)

#define UA_PANEL_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CC_TYPE_UA_PANEL, CcUaPanelPrivate))

struct _CcUaPanelPrivate
{
  GtkBuilder *builder;
  GSettings *wm_settings;
  GSettings *interface_settings;
  GSettings *kb_settings;
  GSettings *mouse_settings;
  GSettings *application_settings;
  GSettings *mediakeys_settings;

  ZoomOptions *zoom_options;
  guint shell_watch_id;
};


static void
cc_ua_panel_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cc_ua_panel_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
cc_ua_panel_dispose (GObject *object)
{
  CcUaPanelPrivate *priv = CC_UA_PANEL (object)->priv;

  if (priv->shell_watch_id)
    {
      g_bus_unwatch_name (priv->shell_watch_id);
      priv->shell_watch_id = 0;
    }

  if (priv->builder)
    {
      g_object_unref (priv->builder);
      priv->builder = NULL;
    }

  if (priv->wm_settings)
    {
      g_object_unref (priv->wm_settings);
      priv->wm_settings = NULL;
    }

  if (priv->interface_settings)
    {
      g_object_unref (priv->interface_settings);
      priv->interface_settings = NULL;
    }

  if (priv->kb_settings)
    {
      g_object_unref (priv->kb_settings);
      priv->kb_settings = NULL;
    }

  if (priv->mouse_settings)
    {
      g_object_unref (priv->mouse_settings);
      priv->mouse_settings = NULL;
    }

  if (priv->application_settings)
    {
      g_object_unref (priv->application_settings);
      priv->application_settings = NULL;
    }

  if (priv->mediakeys_settings)
    {
      g_object_unref (priv->mediakeys_settings);
      priv->mediakeys_settings = NULL;
    }

  if (priv->zoom_options)
    {
      g_object_unref (priv->zoom_options);
      priv->zoom_options = NULL;
    }

  G_OBJECT_CLASS (cc_ua_panel_parent_class)->dispose (object);
}

static void
cc_ua_panel_finalize (GObject *object)
{
  G_OBJECT_CLASS (cc_ua_panel_parent_class)->finalize (object);
}

static void
cc_ua_panel_class_init (CcUaPanelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (CcUaPanelPrivate));

  object_class->get_property = cc_ua_panel_get_property;
  object_class->set_property = cc_ua_panel_set_property;
  object_class->dispose = cc_ua_panel_dispose;
  object_class->finalize = cc_ua_panel_finalize;
}

static void
cc_ua_panel_class_finalize (CcUaPanelClass *klass)
{
}

static gchar *typing_assistant_section[] = {
    "typing_assistant_preferences_button",
    NULL
};

static gchar *sticky_keys_section[] = {
    "typing_sticky_keys_disable_two_keys_checkbutton",
    "typing_sticky_keys_beep_modifier_checkbutton",
    NULL
};

static gchar *slow_keys_section[]= {
    "typing_slowkeys_delay_box",
    "typing_slow_keys_beeb_box",
    NULL
};

static gchar *bounce_keys_section[] = {
    "typing_bouncekeys_delay_box",
    "typing_bounce_keys_beep_rejected_checkbutton",
    NULL
};

static gchar *secondary_click_section[] = {
    "pointing_secondary_click_scale_box",
    NULL
};

static gchar *dwell_click_section[] = {
    "pointing_hover_click_delay_scale_box",
    "pointing_hover_click_threshold_scale_box",
    NULL
};

static gchar *visual_alerts_section[] = {
    "hearing_test_flash_button",
    "hearing_flash_window_title_button",
    "hearing_flash_screen_button",
    NULL
};

/* zoom options dialog */
static void
zoom_options_launch_cb (GtkWidget *options_button, CcUaPanel *self)
{
  if (self->priv->zoom_options == NULL)
    self->priv->zoom_options = zoom_options_new ();

  if (self->priv->zoom_options != NULL)
    zoom_options_set_parent (self->priv->zoom_options,
			     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))));
}

static void
cc_ua_panel_section_switched (GObject    *object,
                              GParamSpec *pspec,
                              GtkBuilder *builder)
{
  GtkWidget *w;
  gboolean enabled;
  gchar **widgets, **s;

  widgets = g_object_get_data (object, "section-widgets");

  g_object_get (object, "active", &enabled, NULL);

  for (s = widgets; *s; s++)
    {
      w = WID (builder, *s);
      gtk_widget_set_sensitive (w, enabled);
    }
}

static void
settings_on_off_editor_new (CcUaPanelPrivate  *priv,
                            GSettings         *settings,
                            const gchar       *key,
                            GtkWidget         *widget,
                            gchar            **section)
{
  /* set data to enable/disable the section this on/off switch controls */
  if (section)
    {
      g_object_set_data (G_OBJECT (widget), "section-widgets", section);
      g_signal_connect (widget, "notify::active",
                        G_CALLBACK (cc_ua_panel_section_switched),
                        priv->builder);
    }

  /* set up the boolean editor */
  g_settings_bind (settings, key, widget, "active", G_SETTINGS_BIND_DEFAULT);
}

/* seeing section */
#define GTK_THEME_KEY "gtk-theme"
#define ICON_THEME_KEY "icon-theme"
#define CONTRAST_MODEL_THEME_COLUMN 2
#define DPI_MODEL_FACTOR_COLUMN 2
#define DPI_MODEL_FACTOR_CALC_COLUMN 3

static void text_scaling_factor_combo_box_changed (GtkComboBox *box, CcUaPanel *panel);

static void
text_scaling_factor_notify_cb (GSettings   *settings,
			       const gchar *key,
			       CcUaPanel   *panel)
{
  CcUaPanelPrivate *priv = panel->priv;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkWidget *combo;
  gboolean valid;
  gdouble conf_value;
  GtkTreeIter best;
  gdouble distance;

  if (!g_str_equal (key, "text-scaling-factor"))
    return;

  conf_value = g_settings_get_double (settings, key);

  combo = WID (priv->builder, "seeing_text_size_combobox");
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

  /* Recalculate the font sizes so that
   * their size is about constant when changing text size */
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      gfloat factor;

      gtk_tree_model_get (model, &iter,
                          DPI_MODEL_FACTOR_COLUMN, &factor,
                          -1);

      factor /= conf_value;

      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                          DPI_MODEL_FACTOR_CALC_COLUMN, factor,
                          -1);

      valid = gtk_tree_model_iter_next (model, &iter);
    }

  /* find the closest match in the combobox model */
  distance = 1e6;
  valid = gtk_tree_model_get_iter_first (model, &iter);
  while (valid)
    {
      gfloat factor;
      gdouble d;

      gtk_tree_model_get (model, &iter,
                          DPI_MODEL_FACTOR_COLUMN, &factor,
                          -1);

      d = fabs (conf_value - factor);
      if (d < distance)
        {
          best = iter;
          distance = d;
        }

      valid = gtk_tree_model_iter_next (model, &iter);
    }

  g_signal_handlers_block_by_func (combo, text_scaling_factor_combo_box_changed, panel);
  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &best);
  g_signal_handlers_unblock_by_func (combo, text_scaling_factor_combo_box_changed, panel);
}

static void
text_scaling_factor_combo_box_changed (GtkComboBox *box,
				       CcUaPanel *panel)
{
  CcUaPanelPrivate *priv = panel->priv;
  GtkTreeIter iter;
  gfloat factor;

  gtk_combo_box_get_active_iter (box, &iter);

  gtk_tree_model_get (gtk_combo_box_get_model (box), &iter,
                      DPI_MODEL_FACTOR_COLUMN, &factor,
                      -1);

  if (factor == 1.0)
    g_settings_reset (priv->interface_settings, "text-scaling-factor");
  else
    g_settings_set_double (priv->interface_settings, "text-scaling-factor", factor);
}


static void
interface_settings_changed_cb (GSettings   *settings,
                               const gchar *key,
                               CcUaPanel   *panel)
{
  CcUaPanelPrivate *priv = panel->priv;

  if (g_str_equal (key, "gtk-theme")) {
    GtkTreeIter iter;
    GtkTreeModel *model;
    GtkWidget *combo;
    gboolean valid;
    gchar *theme_value;

    theme_value = g_settings_get_string (settings, GTK_THEME_KEY);

    combo = WID (priv->builder, "seeing_contrast_combobox");
    model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

    /* see if there is a matching theme name in the combobox model */
    valid = gtk_tree_model_get_iter_first (model, &iter);
    while (valid)
      {
        gchar *value;

        gtk_tree_model_get (model, &iter,
                            CONTRAST_MODEL_THEME_COLUMN, &value,
                            -1);

        if (!g_strcmp0 (value, theme_value))
          {
            gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
            g_free (value);
            break;
          }

        g_free (value);
        valid = gtk_tree_model_iter_next (model, &iter);
      }

    /* if a value for the current theme was not found in the combobox, set to the
     * "normal" option */
    if (!valid)
      {
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 1);
      }
  }
}

static void
contrast_combobox_changed_cb (GtkComboBox *box,
                              CcUaPanel   *panel)
{
  CcUaPanelPrivate *priv = panel->priv;
  gchar *theme_name = NULL;
  GtkTreeIter iter;

  gtk_combo_box_get_active_iter (box, &iter);

  gtk_tree_model_get (gtk_combo_box_get_model (box), &iter,
                      CONTRAST_MODEL_THEME_COLUMN, &theme_name,
                      -1);

  if (g_strcmp0 (theme_name, ""))
    {
      g_settings_set_string (priv->interface_settings, GTK_THEME_KEY, theme_name);
      g_settings_set_string (priv->interface_settings, ICON_THEME_KEY, theme_name);
    }
  else
    {
      g_settings_reset (priv->interface_settings, GTK_THEME_KEY);
      g_settings_reset (priv->interface_settings, ICON_THEME_KEY);
    }

  g_free (theme_name);
}

static void
cc_ua_panel_set_shortcut_label (CcUaPanel  *self,
				const char *label,
				const char *key)
{
	GtkWidget *widget;
	char *value;
	char *text;
	guint accel_key, *keycode;
	GdkModifierType mods;

	widget = WID (self->priv->builder, label);
	value = g_settings_get_string (self->priv->mediakeys_settings, key);

	if (value == NULL || *value == '\0') {
		gtk_label_set_text (GTK_LABEL (widget), _("No shortcut set"));
		g_free (value);
		return;
	}
	gtk_accelerator_parse_with_keycode (value, &accel_key, &keycode, &mods);
	if (accel_key == 0 && keycode == NULL && mods == 0) {
		gtk_label_set_text (GTK_LABEL (widget), _("No shortcut set"));
		g_free (value);
		g_warning ("Failed to parse keyboard shortcut: '%s'", value);
		return;
	}
	g_free (value);

	text = gtk_accelerator_get_label_with_keycode (gtk_widget_get_display (widget), accel_key, *keycode, mods);
	g_free (keycode);
	gtk_label_set_text (GTK_LABEL (widget), text);
	g_free (text);
}

static void
shell_vanished_cb (GDBusConnection *connection,
		   const gchar *name,
		   CcUaPanel   *self)
{
  CcUaPanelPrivate *priv = self->priv;

  gtk_widget_hide (WID (priv->builder, "zoom_frame"));
}

static void
shell_appeared_cb (GDBusConnection *connection,
		   const gchar *name,
		   const gchar *name_owner,
		   CcUaPanel   *self)
{
  CcUaPanelPrivate *priv = self->priv;

  gtk_widget_show (WID (priv->builder, "zoom_frame"));
}

static void
cc_ua_panel_init_seeing (CcUaPanel *self)
{
  CcUaPanelPrivate *priv = self->priv;

  text_scaling_factor_notify_cb (priv->interface_settings, "text-scaling-factor", self);
  g_signal_connect (priv->interface_settings, "changed::text-scaling-factor", G_CALLBACK (text_scaling_factor_notify_cb), self);

  g_signal_connect (WID (priv->builder, "seeing_contrast_combobox"), "changed",
                    G_CALLBACK (contrast_combobox_changed_cb), self);

  g_signal_connect (WID (priv->builder, "seeing_text_size_combobox"), "changed",
                    G_CALLBACK (text_scaling_factor_combo_box_changed), self);

  g_settings_bind (priv->kb_settings, "togglekeys-enable",
                   WID (priv->builder, "seeing_enable_toggle_keys_checkbutton"), "active",
                   G_SETTINGS_BIND_DEFAULT);

  priv->shell_watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
					   "org.gnome.Shell",
					   G_BUS_NAME_WATCHER_FLAGS_NONE,
					   (GBusNameAppearedCallback) shell_appeared_cb,
					   (GBusNameVanishedCallback) shell_vanished_cb,
					   self,
					   NULL);
  g_signal_connect (WID (priv->builder, "seeing_zoom_preferences_button"),
                    "clicked",
                    G_CALLBACK (zoom_options_launch_cb), self);
  g_settings_bind (priv->application_settings, "screen-magnifier-enabled",
                   WID (priv->builder, "seeing_zoom_switch"), "active",
                   G_SETTINGS_BIND_DEFAULT);

  settings_on_off_editor_new (priv, priv->application_settings,
                              "screen-reader-enabled",
                              WID (priv->builder, "seeing_reader_switch"),
                              NULL);

  cc_ua_panel_set_shortcut_label (self, "seeing_contrast_toggle_keybinding_label", "toggle-contrast");
  cc_ua_panel_set_shortcut_label (self, "seeing_increase_size_keybinding_label", "increase-text-size");
  cc_ua_panel_set_shortcut_label (self, "seeing_decrease_size_keybinding_label", "decrease-text-size");
  cc_ua_panel_set_shortcut_label (self, "seeing_zoom_enable_keybinding_label", "magnifier");
  cc_ua_panel_set_shortcut_label (self, "seeing_zoom_in_keybinding_label", "magnifier-zoom-in");
  cc_ua_panel_set_shortcut_label (self, "seeing_zoom_out_keybinding_label", "magnifier-zoom-out");
  cc_ua_panel_set_shortcut_label (self, "seeing_reader_enable_keybinding_label", "screenreader");
}


/* hearing/sound section */
static void
visual_bell_type_notify_cb (GSettings   *settings,
                            const gchar *key,
                            CcUaPanel   *panel)
{
  GtkWidget *widget;
  GDesktopVisualBellType type;

  type = g_settings_get_enum (panel->priv->wm_settings, "visual-bell-type");

  if (type == G_DESKTOP_VISUAL_BELL_FRAME_FLASH)
    widget = WID (panel->priv->builder, "hearing_flash_window_title_button");
  else
    widget = WID (panel->priv->builder, "hearing_flash_screen_button");

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
}

static void
visual_bell_type_toggle_cb (GtkWidget *button,
                            CcUaPanel *panel)
{
  gboolean frame_flash;
  GDesktopVisualBellType type;

  frame_flash = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

  if (frame_flash)
    type = G_DESKTOP_VISUAL_BELL_FRAME_FLASH;
  else
    type = G_DESKTOP_VISUAL_BELL_FULLSCREEN_FLASH;
  g_settings_set_enum (panel->priv->wm_settings, "visual-bell-type", type);
}

static gboolean
hearing_sound_preferences_clicked (GtkButton  *button,
                                   CcUaPanel  *panel)
{
  CcShell *shell;

  shell = cc_panel_get_shell (CC_PANEL (panel));
  cc_shell_set_active_panel_from_id (shell, "sound", NULL, NULL);

  return TRUE;
}

static void
cc_ua_panel_init_hearing (CcUaPanel *self)
{
  CcUaPanelPrivate *priv = self->priv;
  GtkWidget *w;

  /* set the initial visual bell values */
  visual_bell_type_notify_cb (NULL, NULL, self);

  /* and listen */
  w = WID (priv->builder, "hearing_visual_alerts_switch");
  settings_on_off_editor_new (priv, priv->wm_settings, "visual-bell", w, visual_alerts_section);

  g_signal_connect (priv->wm_settings, "changed::visual-bell-type",
                    G_CALLBACK (visual_bell_type_notify_cb), self);
  g_signal_connect (WID (priv->builder, "hearing_flash_window_title_button"),
                    "toggled", G_CALLBACK (visual_bell_type_toggle_cb), self);

  /* test flash */
  g_signal_connect (WID (priv->builder, "hearing_test_flash_button"),
                    "clicked", G_CALLBACK (gdk_beep), NULL);

  g_signal_connect (WID (priv->builder, "hearing_sound_preferences_link"),
                    "activate-link",
                    G_CALLBACK (hearing_sound_preferences_clicked), self);
}

/* typing/keyboard section */
static gboolean
typing_keyboard_preferences_clicked (GtkButton  *button,
                                     CcUaPanel  *panel)
{
  CcShell *shell;

  shell = cc_panel_get_shell (CC_PANEL (panel));
  cc_shell_set_active_panel_from_id (shell, "keyboard", NULL, NULL);

  return TRUE;
}

static void
cc_ua_panel_init_keyboard (CcUaPanel *self)
{
  CcUaPanelPrivate *priv = self->priv;
  GtkWidget *w;

  /* Typing assistant (on-screen keyboard) */
  w = WID (priv->builder, "typing_assistant_switch");
  settings_on_off_editor_new (priv, priv->application_settings, "screen-keyboard-enabled", w, typing_assistant_section);

  /* enable shortcuts */
  w = WID (priv->builder, "typing_keyboard_toggle_checkbox");
  g_settings_bind (priv->kb_settings, "enable", w, "active", G_SETTINGS_BIND_DEFAULT);

  /* sticky keys */
  w = WID (priv->builder, "typing_sticky_keys_switch");
  settings_on_off_editor_new (priv, priv->kb_settings, "stickykeys-enable", w, sticky_keys_section);

  w = WID (priv->builder, "typing_sticky_keys_disable_two_keys_checkbutton");
  g_settings_bind (priv->kb_settings, "stickykeys-two-key-off", w, "active", G_SETTINGS_BIND_NO_SENSITIVITY);

  w = WID (priv->builder, "typing_sticky_keys_beep_modifier_checkbutton");
  g_settings_bind (priv->kb_settings, "stickykeys-modifier-beep", w, "active", G_SETTINGS_BIND_NO_SENSITIVITY);

  /* slow keys */
  w = WID (priv->builder, "typing_slow_keys_switch");
  settings_on_off_editor_new (priv, priv->kb_settings, "slowkeys-enable", w, slow_keys_section);

  w = WID (priv->builder, "typing_slowkeys_delay_scale");
  g_settings_bind (priv->kb_settings, "slowkeys-delay",
                   gtk_range_get_adjustment (GTK_RANGE (w)), "value",
                   G_SETTINGS_BIND_DEFAULT);

  w = WID (priv->builder, "typing_slow_keys_beep_pressed_checkbutton");
  g_settings_bind (priv->kb_settings, "slowkeys-beep-press", w, "active", G_SETTINGS_BIND_DEFAULT);

  w = WID (priv->builder, "typing_slow_keys_beep_accepted_checkbutton");
  g_settings_bind (priv->kb_settings, "slowkeys-beep-accept", w, "active", G_SETTINGS_BIND_DEFAULT);

  w = WID (priv->builder, "typing_slow_keys_beep_rejected_checkbutton");
  g_settings_bind (priv->kb_settings, "slowkeys-beep-reject", w, "active", G_SETTINGS_BIND_DEFAULT);

  /* bounce keys */
  w = WID (priv->builder, "typing_bounce_keys_switch");
  settings_on_off_editor_new (priv, priv->kb_settings, "bouncekeys-enable", w, bounce_keys_section);

  w = WID (priv->builder, "typing_bouncekeys_delay_scale");
  g_settings_bind (priv->kb_settings, "bouncekeys-delay",
                   gtk_range_get_adjustment (GTK_RANGE (w)), "value",
                   G_SETTINGS_BIND_DEFAULT);

  w = WID (priv->builder, "typing_bounce_keys_beep_rejected_checkbutton");
  g_settings_bind (priv->kb_settings, "bouncekeys-beep-reject", w, "active", G_SETTINGS_BIND_NO_SENSITIVITY);

  g_signal_connect (WID (priv->builder, "typing_keyboard_preferences_link"),
                    "activate-link",
                    G_CALLBACK (typing_keyboard_preferences_clicked), self);
}

/* mouse/pointing & clicking section */
static gboolean
pointing_mouse_preferences_clicked_cb (GtkButton  *button,
                                       CcUaPanel  *panel)
{
  CcShell *shell;

  shell = cc_panel_get_shell (CC_PANEL (panel));
  cc_shell_set_active_panel_from_id (shell, "mouse", NULL, NULL);

  return TRUE;
}

static void
cc_ua_panel_init_mouse (CcUaPanel *self)
{
  CcUaPanelPrivate *priv = self->priv;
  GtkWidget *w;

  /* mouse keys */
  w = WID (priv->builder, "pointing_mouse_keys_switch");
  settings_on_off_editor_new (priv, priv->kb_settings, "mousekeys-enable", w, NULL);

  /* simulated secondary click */
  w = WID (priv->builder, "pointing_second_click_switch");
  settings_on_off_editor_new (priv, priv->mouse_settings, "secondary-click-enabled", w, secondary_click_section);

  w = WID (priv->builder, "pointing_secondary_click_delay_scale");
  g_settings_bind (priv->mouse_settings, "secondary-click-time",
                   gtk_range_get_adjustment (GTK_RANGE (w)), "value",
                   G_SETTINGS_BIND_DEFAULT);

  /* dwell click */
  w = WID (priv->builder, "pointing_hover_click_switch");
  settings_on_off_editor_new (priv, priv->mouse_settings, "dwell-click-enabled", w, dwell_click_section);

  w = WID (priv->builder, "pointing_dwell_delay_scale");
  g_settings_bind (priv->mouse_settings, "dwell-time",
                   gtk_range_get_adjustment (GTK_RANGE (w)), "value",
                   G_SETTINGS_BIND_DEFAULT);

  w = WID (priv->builder, "pointing_dwell_threshold_scale");
  g_settings_bind (priv->mouse_settings, "dwell-threshold",
                   gtk_range_get_adjustment (GTK_RANGE (w)), "value",
                   G_SETTINGS_BIND_DEFAULT);

  /* mouse preferences button */
  g_signal_connect (WID (priv->builder, "pointing_mouse_preferences_link"),
                    "activate-link",
                    G_CALLBACK (pointing_mouse_preferences_clicked_cb), self);
}

static void
cc_ua_panel_init (CcUaPanel *self)
{
  CcUaPanelPrivate *priv;
  GtkWidget *widget;
  GError *err = NULL;
  gchar *objects[] = { "universal_access_box", "contrast_model",
                       "text_size_model", "slowkeys_delay_adjustment",
                       "bouncekeys_delay_adjustment", "click_delay_adjustment",
                       "dwell_time_adjustment", "dwell_threshold_adjustment",
                       "seeing_sizegroup", "typing_sizegroup",
                       "pointing_sizegroup", "pointing_sizegroup2",
                       "pointing_scale_sizegroup", "sizegroup1",
                       "hearing_sizegroup",
                       NULL };

  priv = self->priv = UA_PANEL_PRIVATE (self);

  priv->builder = gtk_builder_new ();

  gtk_builder_add_objects_from_file (priv->builder,
                                     GNOMECC_UI_DIR "/uap.ui",
                                     objects,
                                     &err);

  if (err)
    {
      g_warning ("Could not load interface file: %s", err->message);
      g_error_free (err);

      g_object_unref (priv->builder);
      priv->builder = NULL;

      return;
    }

  priv->interface_settings = g_settings_new ("org.gnome.desktop.interface");
  g_signal_connect (priv->interface_settings, "changed",
                    G_CALLBACK (interface_settings_changed_cb), self);
  interface_settings_changed_cb (priv->interface_settings, "gtk-theme", self);

  priv->wm_settings = g_settings_new ("org.gnome.desktop.wm.preferences");
  priv->kb_settings = g_settings_new ("org.gnome.desktop.a11y.keyboard");
  priv->mouse_settings = g_settings_new ("org.gnome.desktop.a11y.mouse");
  priv->application_settings = g_settings_new ("org.gnome.desktop.a11y.applications");
  priv->mediakeys_settings = g_settings_new ("org.gnome.settings-daemon.plugins.media-keys");

  cc_ua_panel_init_keyboard (self);
  cc_ua_panel_init_mouse (self);
  cc_ua_panel_init_hearing (self);
  cc_ua_panel_init_seeing (self);

  widget = (GtkWidget*) gtk_builder_get_object (priv->builder,
                                                "universal_access_box");

  gtk_container_add (GTK_CONTAINER (self), widget);
}

void
cc_ua_panel_register (GIOModule *module)
{
  cc_ua_panel_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (CC_SHELL_PANEL_EXTENSION_POINT,
                                  CC_TYPE_UA_PANEL,
                                  "universal-access", 0);
}

