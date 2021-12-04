/*
 *  Copyright (C) 2015-2021 Gooroom <gooroom@gooroom.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <gdk/gdkkeysyms.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include <libgnome-panel/gp-applet.h>
#include <libnotify/notify.h>

#define GNOME_DESKTOP_USE_UNSTABLE_API
#include <libgnome-desktop/gnome-languages.h>


#include "common.h"
#include "popup-window.h"
#include "gooroom-integration-applet.h"

#include "modules/user/user-module.h"
#include "modules/sound/sound-module.h"
#include "modules/power/power-module.h"
#include "modules/datetime/datetime-module.h"
#include "modules/security/security-module.h"
#include "modules/endsession/endsession-module.h"
#include "modules/nimf/nimf-module.h"



struct _GooroomIntegrationAppletPrivate
{
	PopupWindow      *popup;
	GtkWidget        *button;

	UserModule       *user_module;
	SoundModule      *sound_module;
	SecurityModule   *sec_module;
	PowerModule      *power_module;
	DateTimeModule   *datetime_module;
	EndSessionModule *endsession_module;
	UpdaterModule    *updater_module;
	NimfModule       *nimf_module;
};



G_DEFINE_TYPE_WITH_PRIVATE (GooroomIntegrationApplet, gooroom_integration_applet, GP_TYPE_APPLET)



static void
get_monitor_geometry (GooroomIntegrationApplet *applet,
                      GdkRectangle             *geometry)
{
	GdkDisplay *d;
	GdkWindow  *w;
	GdkMonitor *m;

	d = gdk_display_get_default ();
	w = gtk_widget_get_window (applet->priv->button);
	m = gdk_display_get_monitor_at_window (d, w);

	gdk_monitor_get_geometry (m, geometry);
}

static void
get_workarea (GooroomIntegrationApplet *applet, GdkRectangle *workarea)
{
	GtkOrientation orientation;
	gint applet_width = 0, applet_height = 0;

	get_monitor_geometry (applet, workarea);

	gtk_widget_get_preferred_width (GTK_WIDGET (applet), NULL, &applet_width);
	gtk_widget_get_preferred_height (GTK_WIDGET (applet), NULL, &applet_height);

	orientation = gp_applet_get_orientation (GP_APPLET (applet));

	switch (orientation) {
		case GTK_ORIENTATION_HORIZONTAL:
			workarea->y += applet_height;
			workarea->height -= applet_height;
		break;

		case GTK_ORIENTATION_VERTICAL:
			workarea->x += applet_width;
			workarea->width -= applet_width;
		break;

		default:
			g_assert_not_reached ();
	}
}


/* Copied from gnome-panel-3.30.3/gnome-panel/libpanel-util/panel-launch.c:
 * dummy_child_watch () */
static void
dummy_child_watch (GPid     pid,
                   gint     status,
                   gpointer user_data)
{
  /* Nothing, this is just to ensure we don't double fork
   * and break pkexec:
   * https://bugzilla.gnome.org/show_bug.cgi?id=675789
   */
}

static void
destroy_popup_window (GooroomIntegrationApplet *applet)
{
	if (applet->priv->popup) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (applet->priv->button), FALSE);
		gtk_widget_destroy (GTK_WIDGET (applet->priv->popup));
		applet->priv->popup = NULL;
	}
}

static void
show_error_dialog (GtkWindow *parent,
                   GdkScreen *screen,
                   const char *primary_text)
{
	gchar *msg = NULL;
	GtkWidget *dialog;

	if (!primary_text) {
		msg = g_markup_printf_escaped ("An Error occurred while trying to launch application");
		primary_text = msg;
	}

	dialog = gtk_message_dialog_new (NULL,
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     NULL);
	if (screen)
		gtk_window_set_screen (GTK_WINDOW (dialog), screen);

	if (!parent)
		gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), FALSE);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", primary_text);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Application Launching Error"));
	gtk_widget_show_all (dialog);

	g_signal_connect_swapped (G_OBJECT (dialog), "response",
                              G_CALLBACK (gtk_widget_destroy), G_OBJECT (dialog));

	g_free (msg);
}

static gchar *
get_desktop_name (const gchar *desktop_id)
{
	gchar *id, *key;
	gchar *name = NULL;
	gchar *curr_locale, *lang_code;
	GKeyFile *keyfile;

	if (!desktop_id)
		return NULL;

	if (g_path_is_absolute (desktop_id)) {
		id = g_strdup (desktop_id);
	} else {
		id = g_strdup_printf ("/usr/share/applications/%s", desktop_id);
	}

	curr_locale = g_strdup (setlocale (LC_MESSAGES, NULL));
	gnome_parse_locale (curr_locale, &lang_code, NULL, NULL, NULL);

	key = g_strdup_printf ("Name[%s]", lang_code);

	keyfile = g_key_file_new ();

	if (g_key_file_load_from_file (keyfile, id,
                                   G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                   NULL)) {
		name = g_key_file_get_string (keyfile, "Desktop Entry", key, NULL);
		if (!name) {
			name = g_key_file_get_string (keyfile, "Desktop Entry", "Name", NULL);
		}
	}

	g_key_file_free (keyfile);
	g_free (curr_locale);
	g_free (lang_code);
	g_free (id);
	g_free (key);

	return name;
}

static void
_g_spawn_async (const gchar *command)
{
	GPid pid;
	gboolean res = FALSE;
	char   **argv  = NULL;
	GError  *error = NULL;

	res = g_shell_parse_argv (command, NULL, &argv, &error);
	if (!res) {
		g_warning ("Could not parse command: %s", error->message);
		g_error_free (error);
		return;
	}

	g_spawn_async (NULL, /*working directory */
			argv,
			NULL, /* envp */
			G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
			NULL,
			NULL,
			&pid,
			&error);

	if (error == NULL) {
		g_child_watch_add (pid, dummy_child_watch, NULL);
	}

	g_strfreev (argv);

	if (error) {
		g_warning ("Could not run command: %s", error->message);
		g_error_free (error);
	}
}
static void
on_destroy_popup_cb (GObject *object, gpointer data)
{
	GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (data);

	destroy_popup_window (applet);
}

static void
on_launch_command_cb (GObject *object, const gchar *command, gpointer data)
{
	GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (data);
	GooroomIntegrationAppletPrivate *priv = applet->priv;

	gchar **argv;
	gboolean res = FALSE;

	if (g_shell_parse_argv (command, NULL, &argv, NULL)) {
		gchar *cmd = g_find_program_in_path (argv[0]);
		if (cmd) {
			_g_spawn_async (command);
			g_free (cmd);
			res = TRUE;
		}
	}

	if (!res) {
		const gchar *msg = _("Could not execute command");
		show_error_dialog (NULL, gtk_widget_get_screen (GTK_WIDGET (priv->button)), msg);
	}

	destroy_popup_window (applet);
}

static void
on_change_engine_done_cb (GObject *object, gpointer data)
{
	GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (data);
	GooroomIntegrationAppletPrivate *priv = applet->priv;

	destroy_popup_window (applet);
}

static void
on_launch_desktop_cb (GObject *object, const gchar *desktop_id, gpointer data)
{
	GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (data);
	GooroomIntegrationAppletPrivate *priv = applet->priv;

	destroy_popup_window (applet);

	GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (priv->button));
	if (!launch_desktop_id (desktop_id, screen)) {
		gchar *msg = NULL;
		gchar *name = get_desktop_name (desktop_id);

		if (name) {
			msg = g_markup_printf_escaped (_("Could not launch '%s' program"), name);
		} else {
			msg = g_markup_printf_escaped (_("Could not launch '%s'"), desktop_id);
		}

		show_error_dialog (NULL, screen, msg);
		g_free (msg);
		g_free (name);
	}
}

static void
on_popup_window_closed (PopupWindow *window,
                        gint         reason,
                        gpointer     data)
{
	GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (data);
	GooroomIntegrationAppletPrivate *priv = applet->priv;

	destroy_popup_window (applet);
}

static gboolean
set_popup_window_position (GooroomIntegrationApplet *applet)
{
	GdkRectangle geometry;
	GtkOrientation orientation;
	gint x, y;
	gint popup_width, popup_height;
	gint applet_width, applet_height;

	GooroomIntegrationAppletPrivate *priv = applet->priv;

	g_return_val_if_fail (priv->popup != NULL, FALSE);

	orientation = gp_applet_get_orientation(GP_APPLET (applet));

	gdk_window_get_origin (gtk_widget_get_window (GTK_WIDGET (applet)), &x, &y);

	gtk_widget_get_preferred_width (GTK_WIDGET (applet), NULL, &applet_width);
	gtk_widget_get_preferred_height (GTK_WIDGET (applet), NULL, &applet_height);

	gtk_widget_get_preferred_width (GTK_WIDGET (priv->popup), NULL, &popup_width);
	gtk_widget_get_preferred_height (GTK_WIDGET (priv->popup), NULL, &popup_height);

	get_monitor_geometry (applet, &geometry);

	switch (orientation) {
		case GTK_ORIENTATION_HORIZONTAL:
			if (x + popup_width > geometry.x + geometry.width)
				x -= ((x + popup_width) - (geometry.x + geometry.width));
			//y += applet_height;
			y -= popup_height;
		break;

		case GTK_ORIENTATION_VERTICAL:
			if (y + popup_height > geometry.y + geometry.height)
				y -= ((y + popup_height) - (geometry.y + geometry.height));
			//x += applet_width;
			x -= popup_width;
		break;

		default:
			g_assert_not_reached ();
	}

//	switch (orientation) {
//		case PANEL_APPLET_ORIENT_DOWN:
//			if (x + popup_width > geometry.x + geometry.width)
//				x -= ((x + popup_width) - (geometry.x + geometry.width));
//			y += applet_height;
//			break;
//
//		case PANEL_APPLET_ORIENT_UP:
//			if (x + popup_width > geometry.x + geometry.width)
//				x -= ((x + popup_width) - (geometry.x + geometry.width));
//			y -= popup_height;
//			break;
//
//		case PANEL_APPLET_ORIENT_RIGHT:
//			x += applet_width;
//			if (y + popup_height > geometry.y + geometry.height)
//				y -= ((y + popup_height) - (geometry.y + geometry.height));
//			break;
//
//		case PANEL_APPLET_ORIENT_LEFT:
//			x -= popup_width;
//			if (y + popup_height > geometry.y + geometry.height)
//				y -= ((y + popup_height) - (geometry.y + geometry.height));
//			break;
//
//		default:
//			g_assert_not_reached ();
//	}

	gtk_window_move (GTK_WINDOW (priv->popup), x, y);

	return FALSE;
}

static void
on_popup_window_realized (GtkWidget *widget, gpointer data)
{
	/* make sure the menu is realized to get valid rectangle sizes */
	if (!gtk_widget_get_realized (widget))
		gtk_widget_realize (widget);

	set_popup_window_position (GOOROOM_INTEGRATION_APPLET (data));
}

static void
integration_window_popup (GooroomIntegrationApplet *applet)
{
	GdkRectangle workarea;
	GooroomIntegrationAppletPrivate *priv = applet->priv;

	priv->popup = popup_window_new (GTK_WIDGET (applet));
	gtk_window_set_screen (GTK_WINDOW (priv->popup),
                           gtk_widget_get_screen (GTK_WIDGET (applet)));

	get_workarea (applet, &workarea);
	popup_window_set_workarea (priv->popup, &workarea);

	popup_window_setup_user       (priv->popup, priv->user_module);
	popup_window_setup_sound      (priv->popup, priv->sound_module);
	popup_window_setup_security   (priv->popup, priv->sec_module);
	popup_window_setup_nimf       (priv->popup, priv->nimf_module);
	popup_window_setup_updater    (priv->popup, priv->updater_module);
	popup_window_setup_datetime   (priv->popup, priv->datetime_module);
	popup_window_setup_power      (priv->popup, priv->power_module);
	popup_window_setup_endsession (priv->popup, priv->endsession_module);

	g_signal_connect (G_OBJECT (priv->popup), "realize", G_CALLBACK (on_popup_window_realized), applet);
	g_signal_connect (G_OBJECT (priv->popup), "closed", G_CALLBACK (on_popup_window_closed), applet);
	g_signal_connect (G_OBJECT (priv->popup), "destroy_popup", G_CALLBACK (on_destroy_popup_cb), applet);
	g_signal_connect (G_OBJECT (priv->popup), "launch-command", G_CALLBACK (on_launch_command_cb), applet);
	g_signal_connect (G_OBJECT (priv->popup), "launch-desktop", G_CALLBACK (on_launch_desktop_cb), applet);

	gtk_widget_show (GTK_WIDGET (priv->popup));

	gtk_window_present_with_time (GTK_WINDOW (priv->popup), GDK_CURRENT_TIME);
}

static void
on_applet_button_toggled (GtkToggleButton *button, gpointer data)
{
	GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (data);

	if (gtk_toggle_button_get_active (button))
		integration_window_popup (applet);
}

static void
screen_size_changed_cb (GdkScreen *screen, gpointer data)
{
    GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (data);
    GooroomIntegrationAppletPrivate *priv = applet->priv;

	destroy_popup_window (applet);
}

static void
monitors_changed_cb (GdkScreen *screen, gpointer data)
{
    GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (data);

	destroy_popup_window (applet);
}

static void
gooroom_integration_applet_realize (GtkWidget *widget)
{
    if (GTK_WIDGET_CLASS (gooroom_integration_applet_parent_class)->realize) {
        GTK_WIDGET_CLASS (gooroom_integration_applet_parent_class)->realize (widget);
    }

    g_signal_connect (gtk_widget_get_screen (widget), "size_changed",
                      G_CALLBACK (screen_size_changed_cb), widget);
}

static void
gooroom_integration_applet_unrealize (GtkWidget *widget)
{
	g_signal_handlers_disconnect_by_func (gtk_widget_get_screen (widget),
                                          screen_size_changed_cb, widget);

	if (GTK_WIDGET_CLASS (gooroom_integration_applet_parent_class)->unrealize)
		GTK_WIDGET_CLASS (gooroom_integration_applet_parent_class)->unrealize (widget);
}

static void
gooroom_integration_applet_size_allocate (GtkWidget     *widget,
                                          GtkAllocation *allocation)
{
	gint size;
	GtkOrientation orientation;

	GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (widget);
	GooroomIntegrationAppletPrivate *priv = applet->priv;

	orientation = gp_applet_get_orientation (GP_APPLET (applet));

	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		size = allocation->height;
	else
		size = allocation->width;

	gtk_widget_set_size_request (priv->button, -1, size);

	if (GTK_WIDGET_CLASS (gooroom_integration_applet_parent_class)->size_allocate)
		GTK_WIDGET_CLASS (gooroom_integration_applet_parent_class)->size_allocate (widget, allocation);
}


static void
gooroom_integration_applet_finalize (GObject *object)
{
	GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (object);
	GooroomIntegrationAppletPrivate *priv = applet->priv;

	notify_uninit ();

	if (priv->user_module) g_object_unref (priv->user_module);
	if (priv->sound_module) g_object_unref (priv->sound_module);
	if (priv->sec_module) g_object_unref (priv->sec_module);
	if (priv->nimf_module) g_object_unref (priv->nimf_module);
	if (priv->power_module) g_object_unref (priv->power_module);
	if (priv->datetime_module) g_object_unref (priv->datetime_module);
	if (priv->endsession_module) g_object_unref (priv->endsession_module);
	if (priv->updater_module) g_object_unref (priv->updater_module);

	G_OBJECT_CLASS (gooroom_integration_applet_parent_class)->finalize (object);
}

static gboolean
gooroom_integration_applet_fill (GooroomIntegrationApplet *applet)
{
	g_return_val_if_fail (GP_IS_APPLET (applet), FALSE);

	gtk_widget_show (GTK_WIDGET (applet));

	return TRUE;
}

static void
gooroom_integration_applet_constructed (GObject *object)
{
	GooroomIntegrationApplet *applet = GOOROOM_INTEGRATION_APPLET (object);

	gooroom_integration_applet_fill (applet);
}

static void
gooroom_integration_applet_init (GooroomIntegrationApplet *applet)
{
	GdkDisplay *display;
	GooroomIntegrationAppletPrivate *priv;

	priv = applet->priv = gooroom_integration_applet_get_instance_private (applet);

	gp_applet_set_flags (GP_APPLET (applet), GP_APPLET_FLAGS_EXPAND_MINOR);

	display = gdk_display_get_default ();

	notify_init (PACKAGE_NAME);

	priv->user_module       = user_module_new ();
	priv->sound_module      = sound_module_new ();
	priv->sec_module        = security_module_new ();
	priv->updater_module    = updater_module_new ();
	priv->nimf_module       = nimf_module_new ();
	priv->power_module      = power_module_new ();
	priv->datetime_module   = datetime_module_new ();
	priv->endsession_module = endsession_module_new ();

	priv->button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->button), GTK_RELIEF_NONE);
	gtk_container_add (GTK_CONTAINER (applet), priv->button);
	gtk_widget_show (priv->button);

	GtkWidget *hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20);
	gtk_container_add (GTK_CONTAINER (priv->button), hbox);
	gtk_widget_show (hbox);

	GtkWidget *updater_tray = updater_module_tray_new (priv->updater_module);
	if (updater_tray) {
		gtk_box_pack_start (GTK_BOX (hbox), updater_tray, FALSE, FALSE, 0);
	}

	GtkWidget *nimf_tray = nimf_module_tray_new (priv->nimf_module);
	if (nimf_tray) {
		gtk_box_pack_start (GTK_BOX (hbox), nimf_tray, FALSE, FALSE, 0);
	}

	GtkWidget *power_tray = power_module_tray_new (priv->power_module);
	if (power_tray) {
		gtk_box_pack_start (GTK_BOX (hbox), power_tray, FALSE, FALSE, 0);
	}

	GtkWidget *sound_tray = sound_module_tray_new (priv->sound_module);
	if (sound_tray) {
		gtk_box_pack_start (GTK_BOX (hbox), sound_tray, FALSE, FALSE, 0);
	}

	GtkWidget *sec_tray = security_module_tray_new (priv->sec_module);
	if (sec_tray) {
		gtk_box_pack_start (GTK_BOX (hbox), sec_tray, FALSE, FALSE, 0);
	}

	GtkWidget *dt_tray = datetime_module_tray_new (priv->datetime_module);
	if (dt_tray) {
		gtk_box_pack_start (GTK_BOX (hbox), dt_tray, FALSE, FALSE, 0);
	}

	GtkWidget *user_tray = user_module_tray_new (priv->user_module);
	if (user_tray) {
		gtk_box_pack_start (GTK_BOX (hbox), user_tray, FALSE, FALSE, 0);
	}

	g_signal_connect (G_OBJECT (priv->button), "toggled", G_CALLBACK (on_applet_button_toggled), applet);

	g_signal_connect (G_OBJECT (priv->sec_module), "launch-desktop", G_CALLBACK (on_launch_desktop_cb), applet);
	g_signal_connect (G_OBJECT (priv->updater_module), "destroy-popup", G_CALLBACK (on_destroy_popup_cb), applet);
	g_signal_connect (G_OBJECT (priv->power_module), "launch-desktop", G_CALLBACK (on_launch_desktop_cb), applet);
	g_signal_connect (G_OBJECT (priv->datetime_module), "launch-desktop", G_CALLBACK (on_launch_desktop_cb), applet);
	g_signal_connect (G_OBJECT (priv->nimf_module), "launch-desktop", G_CALLBACK (on_launch_desktop_cb), applet);
	g_signal_connect (G_OBJECT (priv->nimf_module), "change-engine-done", G_CALLBACK (on_change_engine_done_cb), applet);
	g_signal_connect (G_OBJECT (priv->endsession_module), "launch-command", G_CALLBACK (on_launch_command_cb), applet);

	g_signal_connect (gdk_display_get_default_screen (display),
                      "monitors-changed", G_CALLBACK (monitors_changed_cb), applet);
}

static void
gooroom_integration_applet_class_init (GooroomIntegrationAppletClass *class)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;

	object_class = G_OBJECT_CLASS (class);
	widget_class = GTK_WIDGET_CLASS (class);

	object_class->constructed = gooroom_integration_applet_constructed;
	object_class->finalize = gooroom_integration_applet_finalize;

	widget_class->realize       = gooroom_integration_applet_realize;
	widget_class->unrealize     = gooroom_integration_applet_unrealize;
	widget_class->size_allocate = gooroom_integration_applet_size_allocate;
}
