/*
 *  Copyright (C) 2010 Red Hat, Inc
 *  Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
 *  Copyright (C) 2010,2015 Richard Hughes <richard@hughsie.com>
 *  Copyright (C) 2015-2019 Gooroom <gooroom@gooroom.kr>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "common.h"
#include "power-module.h"

#include <glib/gi18n-lib.h>
#include <libupower-glib/upower.h>


struct _PowerModulePrivate
{
	GtkWidget  *tray;
	GtkWidget  *br_control;
	GtkWidget  *br_scale;

	GtkWidget  *bat_control;
	GtkWidget  *bat_desc;
	GtkWidget  *bat_icon;

	UpClient   *up_client;

	GPtrArray  *devices;

	GDBusProxy *screen_proxy;
//	GDBusProxy *kbd_proxy;
};

enum {
	LAUNCH_DESKTOP,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (PowerModule, power_module, G_TYPE_OBJECT)


static void
on_scale_value_changed (GtkRange *range, gpointer data)
{
	PowerModule *module = POWER_MODULE (data);
	PowerModulePrivate *priv = module->priv;

	guint percentage;
	GVariant *variant;
	GDBusProxy *proxy;

	percentage = (guint) gtk_range_get_value (range);
	percentage = (percentage == 0.0) ? 1.0 : percentage;

	if (range == GTK_RANGE (priv->br_scale)) {
		proxy = priv->screen_proxy;

		variant = g_variant_new_parsed ("('org.gnome.SettingsDaemon.Power.Screen',"
                                        "'Brightness', %v)",
                                        g_variant_new_int32 (percentage));
	} else {
#if 0
		proxy = priv->kbd_proxy;

		variant = g_variant_new_parsed ("('org.gnome.SettingsDaemon.Power.Keyboard',"
                                        "'Brightness', %v)",
                                        g_variant_new_int32 (percentage));
#endif
	}

	/* push this to g-s-d */
	g_dbus_proxy_call (proxy,
                       "org.freedesktop.DBus.Properties.Set",
                       variant,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       data);
}

#if 0
static gboolean
on_popup_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	if (event->type == GDK_KEY_PRESS && event->keyval == GDK_Escape) {
		on_popup_window_closed (data);
		return TRUE;
	}

	return FALSE;
}
#endif

/* Copied from gnome-control-center-3.30.3/panel/power/cc-power-panel.c:
 * get_timestring () */
static gchar *
get_timestring (guint64 time_secs)
{
	gchar* timestring = NULL;
	gint  hours;
	gint  minutes;

	/* Add 0.5 to do rounding */
	minutes = (int) ( ( time_secs / 60.0 ) + 0.5 );

	if (minutes == 0) {
		timestring = g_strdup (_("Unknown time"));
		return timestring;
	}

	if (minutes < 60) {
		timestring = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE, "%i minute",
					"%i minutes",
					minutes), minutes);
		return timestring;
	}

	hours = minutes / 60;
	minutes = minutes % 60;

	if (minutes == 0) {
		timestring = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE,
					"%i hour",
					"%i hours",
					hours), hours);
		return timestring;
	}


	/* TRANSLATOR: "%i %s %i %s" are "%i hours %i minutes"
	 * Swap order with "%2$s %2$i %1$s %1$i if needed */
	timestring = g_strdup_printf (_("%i %s %i %s"),
			hours, g_dngettext (GETTEXT_PACKAGE, "hour", "hours", hours),
			minutes, g_dngettext (GETTEXT_PACKAGE, "minute", "minutes", minutes));

	return timestring;
}

/* Copied from gnome-control-center-3.30.3/panel/power/cc-power-panel.c:
 * get_details_string () */
static gchar *
get_details_string (gdouble percentage, UpDeviceState state, guint64 time)
{
  gchar *details;

  if (time > 0)
    {
      gchar *time_string;

      time_string = get_timestring (time);
      switch (state)
        {
          case UP_DEVICE_STATE_CHARGING:
          case UP_DEVICE_STATE_PENDING_CHARGE:
            details = g_strdup_printf (_("%s until fully charged"), time_string);
            break;
          case UP_DEVICE_STATE_DISCHARGING:
          case UP_DEVICE_STATE_PENDING_DISCHARGE:
            if (percentage < 10)
              {
                details = g_strdup_printf (_("Caution: %s remaining"), time_string);
              }
            else
              {
                details = g_strdup_printf (_("%s remaining"), time_string);
              }
            break;
          case UP_DEVICE_STATE_FULLY_CHARGED:
            details = g_strdup (_("Fully charged"));
            break;
          case UP_DEVICE_STATE_EMPTY:
            details = g_strdup (_("Empty"));
            break;
          default:
            details = g_strdup_printf ("error: %s", up_device_state_to_string (state));
            break;
        }
      g_free (time_string);
    }
  else
    {
      switch (state)
        {
          case UP_DEVICE_STATE_CHARGING:
          case UP_DEVICE_STATE_PENDING_CHARGE:
            details = g_strdup (_("Charging"));
            break;
          case UP_DEVICE_STATE_DISCHARGING:
          case UP_DEVICE_STATE_PENDING_DISCHARGE:
            details = g_strdup (_("Discharging"));
            break;
          case UP_DEVICE_STATE_FULLY_CHARGED:
            details = g_strdup (_("Fully charged"));
            break;
          case UP_DEVICE_STATE_EMPTY:
            details = g_strdup (_("Empty"));
            break;
          default:
            details = g_strdup_printf ("error: %s",
                                       up_device_state_to_string (state));
            break;
        }
    }

  return details;
}

static gchar *
get_battery_icon_name (double percentage, UpDeviceState state)
{
	gchar *icon_name = NULL;
	const gchar *bat_state;

	switch (state)
	{
		case UP_DEVICE_STATE_CHARGING:
		case UP_DEVICE_STATE_PENDING_CHARGE:
			bat_state = "-charging";
			break;

		case UP_DEVICE_STATE_DISCHARGING:
		case UP_DEVICE_STATE_PENDING_DISCHARGE:
			bat_state = "";
			break;

		case UP_DEVICE_STATE_FULLY_CHARGED:
			return g_strdup_printf ("battery-full-charged");

		case UP_DEVICE_STATE_EMPTY:
			return g_strdup ("battery-empty");

		default:
			bat_state = NULL;
			break;
	}

	if (!bat_state) {
		return g_strdup ("battery-error");
	}

	if (percentage >= 99) {
		return g_strdup_printf ("battery-full%s", bat_state);
	}
	if (percentage >= 90) {
		return g_strdup_printf ("battery-90%s", bat_state);
	}
	if (percentage >= 75) {
		return g_strdup_printf ("battery-75%s", bat_state);
	}
	if (percentage >= 60) {
		return g_strdup_printf ("battery-60%s", bat_state);
	}
	if (percentage >= 50) {
		return g_strdup_printf ("battery-50%s", bat_state);
	}
	if (percentage >= 40) {
		return g_strdup_printf ("battery-40%s", bat_state);
	}
	if (percentage >= 25) {
		return g_strdup_printf ("battery-25%s", bat_state);
	}
	if (percentage >= 10) {
		return g_strdup_printf ("battery-10%s", bat_state);
	}

	return g_strdup_printf ("battery-empty%s", bat_state);
}

static void
update_primary (UpDevice *device, PowerModule *module)
{
	gchar *icon_name;
	gchar *details1 = NULL, *details2 = NULL;
	gdouble percentage;
	guint64 time_empty, time_full, time;
	UpDeviceState state;
//	gchar *vendor, *model;
	gdouble energy_full, energy_rate;

	PowerModulePrivate *priv = module->priv;

	g_object_get (device,
			"state", &state,
			"percentage", &percentage,
			"time-to-empty", &time_empty,
			"time-to-full", &time_full,
			"energy-full", &energy_full,
			"energy-rate", &energy_rate,
			NULL);

	if (state == UP_DEVICE_STATE_DISCHARGING)
		time = time_empty;
	else
		time = time_full;

	details1 = get_details_string (percentage, state, 0);
	details2 = get_details_string (percentage, state, time);

	icon_name = get_battery_icon_name (percentage, state);

	if (priv->tray && gtk_widget_get_visible (priv->tray)) {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->tray), icon_name, GTK_ICON_SIZE_BUTTON);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
	}

	if (priv->bat_control && gtk_widget_get_visible (priv->bat_control)) {
		if (priv->bat_desc) {
			gchar *s = g_strdup_printf ("<b>%s (%d%%, %s)</b>", details1, (int)(percentage + 0.5), details2);
			gtk_label_set_markup (GTK_LABEL (priv->bat_desc), s);
			g_free (s);
		}

		if (priv->bat_icon) {
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->bat_icon), icon_name, GTK_ICON_SIZE_BUTTON);
			gtk_image_set_pixel_size (GTK_IMAGE (priv->bat_icon), STATUS_ICON_SIZE);
		}
	}

	g_free (details1);
	g_free (details2);
	g_free (icon_name);
}


/* Copied from gnome-control-center-3.30.3/panel/power/cc-power-panel.c:
 * up_client_changed () */
static void
up_client_changed (UpClient *client,
                   UpDevice *device,
                   gpointer  data)
{
	gint i;
	UpDeviceKind kind;
	guint n_batteries;
	gboolean on_ups;
	UpDevice *composite;

	PowerModule *module = POWER_MODULE (data);
	PowerModulePrivate *priv = module->priv;

	on_ups = FALSE;
	n_batteries = 0;
	composite = up_client_get_display_device (priv->up_client);
	g_object_get (composite, "kind", &kind, NULL);

	if (kind == UP_DEVICE_KIND_UPS) {
		on_ups = TRUE;
	} else {
		gboolean is_extra_battery = FALSE;

		/* Count the batteries */
		for (i = 0; priv->devices != NULL && i < priv->devices->len; i++) {
			UpDevice *device = (UpDevice*) g_ptr_array_index (priv->devices, i);
			g_object_get (device, "kind", &kind, NULL);
			if (kind == UP_DEVICE_KIND_BATTERY) {
				n_batteries++;
				if (is_extra_battery == FALSE) {
					is_extra_battery = TRUE;
					g_object_set_data (G_OBJECT (device), "is-main-battery", GINT_TO_POINTER(TRUE));
				}
			}
		}
	}

	if (!on_ups && n_batteries > 1)
		update_primary (composite, module);

	for (i = 0; priv->devices != NULL && i < priv->devices->len; i++) {
		UpDevice *device = (UpDevice*) g_ptr_array_index (priv->devices, i);
		g_object_get (device, "kind", &kind, NULL);

		if (kind == UP_DEVICE_KIND_LINE_POWER) {
		} else if (kind == UP_DEVICE_KIND_UPS && on_ups) {
			update_primary (device, module);
		} else if (kind == UP_DEVICE_KIND_BATTERY && !on_ups && n_batteries == 1) {
			update_primary (device, module);
		} else if (kind == UP_DEVICE_KIND_BATTERY) {
		} else {
		}
	}

	g_object_unref (composite);
}


/* Copied from gnome-control-center-3.30.3/panel/power/cc-power-panel.c:
 * up_client_device_removed () */
static void
up_client_device_removed (UpClient   *client,
                          const char *object_path,
                          gpointer    data)
{
	PowerModule *module = POWER_MODULE (data);
	PowerModulePrivate *priv = module->priv;
	
	if (priv->devices == NULL)
		return;

	guint i;
	for (i = 0; i < priv->devices->len; i++) {
		UpDevice *device = g_ptr_array_index (priv->devices, i);

		if (g_strcmp0 (object_path, up_device_get_object_path (device)) == 0) {
			g_object_unref (device); 
			g_ptr_array_remove_index (priv->devices, i);
			break;
		}
	}

	up_client_changed (priv->up_client, NULL, module);
}

/* Copied from gnome-control-center-3.30.3/panel/power/cc-power-panel.c:
 * up_client_device_added () */
static void
up_client_device_added (UpClient *client,
                        UpDevice *device,
                        gpointer  data)
{
	PowerModule *module = POWER_MODULE (data);
	PowerModulePrivate *priv = module->priv;

	g_ptr_array_add (priv->devices, g_object_ref (device));

	g_signal_connect (G_OBJECT (device), "notify", G_CALLBACK (up_client_changed), module);

	up_client_changed (priv->up_client, NULL, module);
}

static void
on_battery_control_button_clicked_cb (GtkButton *button, gpointer data)
{
	GSettingsSchema *schema;
	PowerModule *module = POWER_MODULE (data);

	schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (), "org.gnome.ControlCenter", TRUE);

	if (schema) {
		GSettings *settings = g_settings_new_full (schema, NULL, NULL);
		g_settings_set_string (settings, "last-panel", "power");
		g_object_unref (settings);
		g_settings_schema_unref (schema);
	}

	g_signal_emit (G_OBJECT (module), signals[LAUNCH_DESKTOP], 0, "gnome-power-panel.desktop");
}

static void
build_battery_control_ui (PowerModule *module, GtkSizeGroup *size_group)
{
    GtkWidget *hbox;
	PowerModulePrivate *priv = module->priv;

	priv->bat_control = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->bat_control), GTK_RELIEF_NONE);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);

	priv->bat_icon = gtk_image_new_from_icon_name ("battery-full-charged",
                                                   GTK_ICON_SIZE_BUTTON);
	if (size_group)
		gtk_size_group_add_widget (size_group, priv->bat_icon);
	gtk_box_pack_start (GTK_BOX (hbox), priv->bat_icon, FALSE, FALSE, 0);

	priv->bat_desc = gtk_label_new ("");
	gtk_label_set_xalign (GTK_LABEL (priv->bat_desc), 0);
	gtk_label_set_max_width_chars (GTK_LABEL (priv->bat_desc), 1);
	gtk_label_set_ellipsize (GTK_LABEL (priv->bat_desc), PANGO_ELLIPSIZE_END);
	gtk_label_set_line_wrap (GTK_LABEL (priv->bat_desc), FALSE);

	gtk_box_pack_start (GTK_BOX (hbox), priv->bat_desc, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (priv->bat_control), hbox);

	g_signal_connect (G_OBJECT (priv->bat_control), "clicked",
                      G_CALLBACK (on_battery_control_button_clicked_cb), module);
}

static void
build_brightness_control_ui (PowerModule *module, GtkSizeGroup *size_group)
{
	GtkWidget *icon;
    GtkWidget *scale;
	PowerModulePrivate *priv = module->priv;

	priv->br_control = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_container_set_border_width (GTK_CONTAINER (priv->br_control), 0);

	icon = gtk_image_new_from_icon_name ("display-brightness-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_pixel_size (GTK_IMAGE (icon), STATUS_ICON_SIZE);
	if (size_group)
		gtk_size_group_add_widget (size_group, icon);
	gtk_box_pack_start (GTK_BOX (priv->br_control), icon, FALSE, FALSE, 0);

	priv->br_scale = scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);

	gtk_range_set_inverted (GTK_RANGE (scale), FALSE);
	gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
	gtk_range_set_round_digits (GTK_RANGE (scale), 0);
	gtk_box_pack_end (GTK_BOX (priv->br_control), scale, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (scale), "value-changed", G_CALLBACK (on_scale_value_changed), module);
}

/* Copied from gnome-control-center-3.30.3/panel/power/cc-power-panel.c:
 * sync_screen_brightness () */
static void
sync_screen_brightness (gpointer data)
{
	PowerModule *module = POWER_MODULE (data);
	PowerModulePrivate *priv = module->priv;

	gint brightness;
	gboolean visible;
	GVariant *result;

	if (priv->br_control) {
		result = g_dbus_proxy_get_cached_property (priv->screen_proxy, "Brightness");
		if (result) {
			/* set the slider */
			brightness = g_variant_get_int32 (result);
			visible = brightness >= 0.0;
		} else {
			visible = FALSE;
		}

		if (visible) {
			gtk_widget_show_all (priv->br_control);

			g_signal_handlers_block_by_func (G_OBJECT (priv->br_scale), on_scale_value_changed, module);
			gtk_range_set_value (GTK_RANGE (priv->br_scale), brightness);
			g_signal_handlers_unblock_by_func (G_OBJECT (priv->br_scale), on_scale_value_changed, module);

			g_variant_unref (result);
		} else {
			gtk_widget_hide (priv->br_control);
		}
	}
}

#if 0
static void
on_screen_property_change (GDBusProxy *proxy,
                           GVariant   *changed_properties,
                           GVariant   *invalidated_properties,
                           gpointer    data)
{
	sync_screen_brightness (data);
}
#endif

/* Copied from gnome-control-center-3.28.2/panel/power/cc-power-panel.c:
 * got_screen_proxy_cb () */
static void
got_screen_proxy_cb (GObject      *source_object,
                     GAsyncResult *res,
                     gpointer      data)
{
	GError *error = NULL;
	GDBusProxy *proxy;

	PowerModule *module = POWER_MODULE (data);
	PowerModulePrivate *priv = module->priv;

	proxy = g_dbus_proxy_new_for_bus_finish (res, &error);
	if (proxy == NULL) {
		if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
			g_printerr ("Error creating screen proxy: %s\n", error->message);
		g_error_free (error);
		return;
	}
	priv->screen_proxy = proxy;

	/* we want to change the bar if the user presses brightness buttons */
//	g_signal_connect (proxy, "g-properties-changed",
//			G_CALLBACK (on_screen_property_change), data);

	sync_screen_brightness (data);
}

static gboolean
has_battery (PowerModule *module)
{
	g_return_val_if_fail (module != NULL, FALSE);

	PowerModulePrivate *priv = module->priv;

	gboolean has_batteries = FALSE;

	GPtrArray *devices = up_client_get_devices2 (priv->up_client);

	guint i;
	for (i = 0; devices != NULL && i < devices->len; i++) {
		UpDeviceKind kind;
		gboolean power_supply;

		UpDevice *device = g_ptr_array_index (devices, i);
		g_object_get (device, 
                      "kind", &kind,
                      "power-supply", &power_supply,
                      NULL);

		if ((kind == UP_DEVICE_KIND_UPS) ||
            (kind == UP_DEVICE_KIND_BATTERY && power_supply)) {
			has_batteries = TRUE;
			break;
		}
	}

	g_clear_pointer (&devices, g_ptr_array_unref);

	return has_batteries;
}

static void
power_module_finalize (GObject *object)
{
	PowerModule *module = POWER_MODULE (object);
	PowerModulePrivate *priv = module->priv;

	power_module_battery_control_destroy (module);
	power_module_brightness_control_destroy (module);

	if (priv->devices) {
		g_ptr_array_foreach (priv->devices, (GFunc) g_object_unref, NULL);
		g_clear_pointer (&priv->devices, g_ptr_array_unref);
	}

	g_clear_object (&priv->up_client);
	g_clear_object (&priv->screen_proxy);

	G_OBJECT_CLASS (power_module_parent_class)->finalize (object);
}

static void
power_module_class_init (PowerModuleClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = power_module_finalize;

	signals[LAUNCH_DESKTOP] = g_signal_new ("launch-desktop",
                                            MODULE_TYPE_POWER,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(PowerModuleClass,
                                            launch_desktop),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);
}

static void
power_module_init (PowerModule *module)
{
	guint i;

	PowerModulePrivate *priv;
	module->priv = priv = power_module_get_instance_private (module);

	priv->tray = NULL;
	priv->up_client = up_client_new ();
	priv->devices   = up_client_get_devices2 (priv->up_client);

	g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
			G_DBUS_PROXY_FLAGS_NONE,
			NULL,
			"org.gnome.SettingsDaemon.Power",
			"/org/gnome/SettingsDaemon/Power",
			"org.gnome.SettingsDaemon.Power.Screen",
			NULL,
			got_screen_proxy_cb,
			module);

#if 0
	g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
			G_DBUS_PROXY_FLAGS_NONE,
			NULL,
			"org.gnome.SettingsDaemon.Power",
			"/org/gnome/SettingsDaemon/Power",
			"org.gnome.SettingsDaemon.Power.Keyboard",
			NULL,
			got_kbd_proxy_cb,
			module);
#endif

	for (i = 0; priv->devices != NULL && i < priv->devices->len; i++) {
		UpDevice *device = g_ptr_array_index (priv->devices, i);

		g_signal_connect (G_OBJECT (device), "notify", G_CALLBACK (up_client_changed), module);
	}

	/* populate batteries */
	g_signal_connect (priv->up_client, "device-added", G_CALLBACK (up_client_device_added), module);
	g_signal_connect (priv->up_client, "device-removed", G_CALLBACK (up_client_device_removed), module);
}

PowerModule *
power_module_new (void)
{
	return g_object_new (MODULE_TYPE_POWER, NULL);
}

GtkWidget *
power_module_tray_new (PowerModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	PowerModulePrivate *priv = module->priv;

	if (has_battery (module)) {
		if (!priv->tray) {
			priv->tray = gtk_image_new_from_icon_name ("battery-full-charged", GTK_ICON_SIZE_BUTTON);
			gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
		}

		gtk_widget_show_all (priv->tray);

		up_client_changed (priv->up_client, NULL, module);
	}/* else {
		gtk_widget_hide (priv->tray);
	}*/

	return priv->tray;
}

GtkWidget *
power_module_brightness_control_new (PowerModule *module, GtkSizeGroup *size_group)
{
	g_return_val_if_fail (module != NULL, NULL);

	PowerModulePrivate *priv = module->priv;

	build_brightness_control_ui (module, size_group);

	gtk_widget_show_all (priv->br_control);

	sync_screen_brightness (module);

	return priv->br_control;
}

GtkWidget *
power_module_battery_control_new (PowerModule *module, GtkSizeGroup *size_group)
{
	g_return_val_if_fail (module != NULL, NULL);

	PowerModulePrivate *priv = module->priv;

	if (has_battery (module)) {
		build_battery_control_ui (module, size_group);
		gtk_widget_show_all (priv->bat_control);

		up_client_changed (priv->up_client, NULL, module);
	}/* else {
		gtk_widget_hide (priv->bat_control);
	}*/

	return priv->bat_control;
}

void
power_module_battery_control_destroy (PowerModule *module)
{
	g_return_if_fail (module != NULL);

	PowerModulePrivate *priv = module->priv;

	if (priv->bat_control) {
		if (priv->bat_desc) {
			gtk_widget_destroy (priv->bat_desc);
			priv->bat_desc = NULL;
		}

		if (priv->bat_icon) {
			gtk_widget_destroy (priv->bat_icon);
			priv->bat_icon = NULL;
		}

		gtk_widget_destroy (priv->bat_control);
		priv->bat_control = NULL;
	}
}

void
power_module_brightness_control_destroy (PowerModule *module)
{
	g_return_if_fail (module != NULL);

	PowerModulePrivate *priv = module->priv;

	if (priv->br_control) {
		gtk_widget_destroy (priv->br_control);
		priv->br_control= NULL;
	}
}
