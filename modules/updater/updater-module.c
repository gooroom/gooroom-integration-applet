/*
 *  Copyright (C) 2015-2021 Gooroom <gooroom@gooroom.kr>
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

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <glib/gstdio.h>

#include "common.h"
#include "updater-module.h"

#define UPDATER_PATH            "/usr/lib/gooroom/gooroomUpdate/gooroomUpdate.py"
#define UPDATER_CONTROL_UI      "/kr/gooroom/IntegrationApplet/modules/updater/updater-control.ui"
#define UPDATER_CONTROL_MENU_UI "/kr/gooroom/IntegrationApplet/modules/updater/updater-control-menu.ui"

#define UPDATER_SERVICE_NAME               "kr.gooroom.Update"
#define UPDATER_SERVICE_PATH               "/kr/gooroom/Update"
#define UPDATER_SERVICE_INTERFACE          "kr.gooroom.Update"

#define GET_WIDGET(builder, x) GTK_WIDGET (gtk_builder_get_object (builder, x))
#define READ_BUFFER_SIZE 4096


struct _UpdaterModulePrivate
{
	GtkWidget	*tray;
	GtkWidget	*control;
	GtkWidget	*lbl_updater_status;
	GtkWidget	*img_status;
	GtkWidget	*box_updater_menu;
	GtkWidget	*btn_view;
	GtkWidget	*btn_pref;
	GtkWidget	*btn_show;

	GtkBuilder	*builder;

	GDBusProxy	*proxy;

	guint        watcher_id;
	gchar       *icon_name;
	gchar       *status_string;
	gboolean     updater_stopped;
};

enum {
	DESTROY_POPUP,
	STATUS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (UpdaterModule, updater_module, G_TYPE_OBJECT)



static void
update_popup_icon (UpdaterModule *module)
{
	UpdaterModulePrivate *priv = module->priv;

	if (priv->control) {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->img_status),
                                      priv->icon_name,
                                      GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->img_status), STATUS_ICON_SIZE);
	}
}

static void
update_tray (UpdaterModule *module)
{
	UpdaterModulePrivate *priv = module->priv;

	if (priv->tray && priv->icon_name) {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->tray), priv->icon_name,
                                      GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
	}
}

static void
update_status_string (UpdaterModule *module)
{
	UpdaterModulePrivate *priv = module->priv;

	if (priv->control) {
		if (priv->status_string) {
			gchar *markup = g_markup_printf_escaped ("%s", priv->status_string);
			gtk_label_set_markup (GTK_LABEL (priv->lbl_updater_status), markup);
			gtk_widget_set_tooltip_text (priv->control, markup);

			g_clear_pointer (&markup, g_free);
		}
	}
}

static gboolean
updater_status_update_idle (gpointer data)
{
	UpdaterModule *module = UPDATER_MODULE (data);

	update_popup_icon (module);
	update_status_string (module);

	return FALSE;
}

static void
check_synaptic (UpdaterModule *module)
{
	UpdaterModulePrivate *priv = module->priv;

	GSubprocess *process;
	GInputStream *stream;

	guchar *buffer;
	gchar *str_output = "";

	buffer = g_new0 (guchar, READ_BUFFER_SIZE);

	process = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE, NULL, "ps", "-U", "root", "-o", "comm", NULL);
	stream = g_subprocess_get_stdout_pipe(process);
	g_input_stream_read(stream, buffer, READ_BUFFER_SIZE, NULL, NULL);

	str_output = g_strdup (buffer);

	if (g_strrstr (str_output, "synaptic") != NULL)
	{
		gchar *markup = g_markup_printf_escaped ("%s", _("Please quit Synaptic to control Update Manager."));
		gtk_widget_set_sensitive (priv->btn_view, FALSE);
		gtk_widget_set_sensitive (priv->btn_pref, FALSE);
		gtk_widget_set_sensitive (priv->btn_show, FALSE);

		g_signal_emit (G_OBJECT (module), signals[STATUS_CHANGED], 0, markup);

		g_free (markup);
	}

	g_free (buffer);
	g_free (str_output);
}

static void
on_show_button_clicked (GtkButton *button, gpointer data)
{
	GError *error = NULL;

	UpdaterModule *module = UPDATER_MODULE (data);
	UpdaterModulePrivate *priv = module->priv;

	if (error) {
		g_warning ("Failed to ..: %s", error->message);
		g_error_free (error);
		return;
	}

	g_dbus_proxy_call (priv->proxy,
                       "Show",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       data);

	g_signal_emit (G_OBJECT (module), signals[DESTROY_POPUP], 0);
}

static void
on_pref_button_clicked (GtkButton *button, gpointer data)
{
	GError *error = NULL;

	UpdaterModule *module = UPDATER_MODULE (data);
	UpdaterModulePrivate *priv = module->priv;

	if (error) {
		g_warning ("Failed to open updater preferences: %s", error->message);
		g_error_free (error);
		return;
	}

	g_dbus_proxy_call (priv->proxy,
                       "Pref",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       data);

	g_signal_emit (G_OBJECT (module), signals[DESTROY_POPUP], 0);
}

static void
on_view_button_clicked (GtkButton *button, gpointer data)
{
	GError *error = NULL;

	UpdaterModule *module = UPDATER_MODULE (data);
	UpdaterModulePrivate *priv = module->priv;

	if (error) {
		g_warning ("Failed to open updater: %s", error->message);
		g_error_free (error);
		return;
	}

	g_dbus_proxy_call (priv->proxy,
                       "Reload",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       NULL,
                       data);

	g_signal_emit (G_OBJECT (module), signals[DESTROY_POPUP], 0);
}

static void
updater_service_signal_cb (GDBusProxy  *proxy,
                           const gchar *sender_name,
                           const gchar *signal_name,
                           GVariant    *parameters,
                           gpointer     data)
{
	UpdaterModule *module = UPDATER_MODULE (data);
	UpdaterModulePrivate *priv = module->priv;

	if (g_strcmp0 (signal_name, "onStatusStringChanged") == 0) {
		const gchar *result = NULL;
		g_variant_get (parameters, "(&s)", &result);
		if (result) {
			g_clear_pointer (&priv->status_string, g_free);
			priv->status_string = g_strdup (result);
			update_status_string (module);       
		}
	}

	if (g_strcmp0 (signal_name, "onIconChanged") == 0) {
		const gchar *icon_path = NULL;
		g_variant_get (parameters, "(&s)", &icon_path);
		if (icon_path) {
			gchar *basename = g_path_get_basename (icon_path);
			if (basename) {
				gchar *p = g_strrstr (basename, ".");
				if (p) *p = 0;  // remove extension
				g_clear_pointer (&priv->icon_name, g_free);
				priv->icon_name = g_strdup_printf ("updater-%s", basename);
				g_free (basename);
			}

			update_tray (module);
			update_popup_icon (module);
		}
	}
}

static void
get_updater_status_str_done_cb (GDBusProxy   *proxy,
                                GAsyncResult *res,
                                gpointer      data)
{
	GError *error = NULL;
	GVariant *result = NULL;
	const gchar *status_string = NULL;
	UpdaterModule *module = UPDATER_MODULE (data);
	UpdaterModulePrivate *priv = module->priv;

	result = g_dbus_proxy_call_finish (proxy, res, &error);

	g_clear_pointer (&priv->status_string, g_free);
	if (error) {
		g_warning ("Failed to get updater status string: %s", error->message);
		g_error_free (error);
		priv->updater_stopped = TRUE;
		priv->status_string = g_strdup (_("Gooroom Update is terminated"));
		goto failed;
	}

	priv->updater_stopped = FALSE;
	if (result) {
		if (g_variant_is_of_type (result, G_VARIANT_TYPE ("(s)")))
			g_variant_get (result, "(&s)", &status_string);
		g_variant_unref (result);
	}

	if (status_string) {
		priv->status_string = g_strdup (status_string);
	} else {
		priv->status_string = g_strdup (_("Gooroom Update is terminated"));
	}

failed:
	update_status_string (module);

	if (priv->control)
		gtk_widget_set_sensitive (priv->control, !priv->updater_stopped);
}

static void
get_updater_icon_done_cb (GDBusProxy   *proxy,
                          GAsyncResult *res,
                          gpointer      data)
{
	GError *error = NULL;
	GVariant *result = NULL;
	gchar *basename = NULL;
	const gchar *icon_path = NULL;
	UpdaterModule *module = UPDATER_MODULE (data);
	UpdaterModulePrivate *priv = module->priv;

	result = g_dbus_proxy_call_finish (proxy, res, &error);

	g_clear_pointer (&priv->icon_name, g_free);

	if (error) {
		g_warning ("Failed to get updater icon : %s", error->message);
		g_error_free (error);
		priv->updater_stopped = TRUE;
		priv->icon_name =  g_strdup ("updater-base-unkown");
		goto failed;
	}

	priv->updater_stopped = FALSE;
	if (result) {
		if (g_variant_is_of_type (result, G_VARIANT_TYPE ("(s)")))
			g_variant_get (result, "(&s)", &icon_path);
		g_variant_unref (result);
	}

	if (icon_path) {
		basename = g_path_get_basename (icon_path);
		if (basename) {
			gchar *p = g_strrstr (basename, ".");
			if (p) *p = 0;  // remove extension
			priv->icon_name = g_strdup_printf ("updater-%s", basename);
			g_free (basename);
		} else {
			priv->icon_name =  g_strdup ("updater-base-unkown");
		}
	} else {
		priv->icon_name =  g_strdup ("updater-base-unkown");
	}


failed:
	update_tray (module);
	update_popup_icon (module);

	if (priv->control)
		gtk_widget_set_sensitive (priv->control, !priv->updater_stopped);
}

static void
name_appeared_cb (GDBusConnection *connection,
                  const gchar     *name,
                  const gchar     *name_owner,
                  gpointer         data)
{
	GError *error = NULL;
	UpdaterModule *module = UPDATER_MODULE (data);
	UpdaterModulePrivate *priv = module->priv;

//	priv->updater_stopped = FALSE;

	g_clear_pointer (&priv->icon_name, g_free);
	g_clear_pointer (&priv->status_string, g_free);
	priv->icon_name = g_strdup ("updater-base-unkown");
	priv->status_string = g_strdup (_("Gooroom Update is terminated"));

	if (priv->proxy) {
		g_signal_handlers_disconnect_by_func (priv->proxy,
                                              G_CALLBACK (updater_service_signal_cb),
                                              module);
		g_clear_object (&priv->proxy);
	}

	priv->proxy = g_dbus_proxy_new_sync (connection,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         UPDATER_SERVICE_NAME,
                                         UPDATER_SERVICE_PATH,
                                         UPDATER_SERVICE_INTERFACE,
                                         NULL, &error);

	if (error) {
		g_warning ("Failed to get updater service proxy: %s", error->message);
		g_error_free (error);
		priv->updater_stopped = TRUE;
		update_tray (module);
		update_popup_icon (module);
		update_status_string (module);
		goto stopped;
	}

	g_dbus_proxy_call (priv->proxy,
                       "GetCurrentIcon",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       (GAsyncReadyCallback) get_updater_icon_done_cb,
                       module);

	g_dbus_proxy_call (priv->proxy,
                       "GetCurrentStatusString",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       (GAsyncReadyCallback) get_updater_status_str_done_cb,
                       module);

	g_signal_connect (priv->proxy, "g-signal", G_CALLBACK (updater_service_signal_cb), module);


stopped:
	if (priv->control)
		gtk_widget_set_sensitive (priv->control, !priv->updater_stopped);
}

static void
name_vanished_cb (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         data)
{
	UpdaterModule *module = UPDATER_MODULE (data);
	UpdaterModulePrivate *priv = module->priv;

//	GStatBuf stat_buf;
//	gboolean has_exec_perm = TRUE;
//	if (g_stat (UPDATER_PATH, &stat_buf) == 0)
//		has_exec_perm = !(stat_buf.st_mode & S_IXOTH);
//	if (!has_exec_perm)
//		priv->status_string = g_strdup (_("Update blocking function has been enabled"));
//	else
//		priv->status_string = g_strdup (_("Gooroom Update is terminated"));

	priv->updater_stopped = TRUE;

	g_clear_pointer (&priv->icon_name, g_free);
	g_clear_pointer (&priv->status_string, g_free);
	priv->icon_name = g_strdup ("updater-base-unkown");
	priv->status_string = g_strdup (_("Gooroom Update is terminated"));

	update_tray (module);
	update_popup_icon (module);
	update_status_string (module);

	if (priv->control)
		gtk_widget_set_sensitive (priv->control, FALSE);

	if (priv->proxy) {
		g_signal_handlers_disconnect_by_func (priv->proxy,
                                              G_CALLBACK (updater_service_signal_cb),
                                              module);
		g_clear_object (&priv->proxy);
	}
}

static void
updater_module_finalize (GObject *object)
{
	UpdaterModule *module = UPDATER_MODULE (object);
	UpdaterModulePrivate *priv = module->priv;

	if (priv->watcher_id) {
		g_bus_unwatch_name (priv->watcher_id);
		priv->watcher_id = 0;
	}

	g_clear_pointer (&priv->icon_name, g_free);
	g_clear_pointer (&priv->status_string, g_free);

	g_clear_object (&priv->builder);
	g_clear_object (&priv->proxy);

	G_OBJECT_CLASS (updater_module_parent_class)->finalize (object);
}

static void
updater_module_class_init (UpdaterModuleClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = updater_module_finalize;

	signals[DESTROY_POPUP] = g_signal_new ("destroy-popup",
                                           MODULE_TYPE_UPDATER,
                                           G_SIGNAL_RUN_LAST,
                                           G_STRUCT_OFFSET(UpdaterModuleClass,
                                           destroy_popup),
                                           NULL, NULL,
                                           g_cclosure_marshal_VOID__VOID,
                                           G_TYPE_NONE, 0,
                                           G_TYPE_NONE);

	signals[STATUS_CHANGED] = g_signal_new ("status-changed",
                                            MODULE_TYPE_UPDATER,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(UpdaterModuleClass,
                                            status_changed),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);
}

static void
updater_module_init (UpdaterModule *module)
{
	UpdaterModulePrivate *priv;

	module->priv = priv = updater_module_get_instance_private (module);

	priv->tray                   = NULL;
	priv->control                = NULL;
	priv->lbl_updater_status     = NULL;
	priv->img_status             = NULL;
	priv->box_updater_menu       = NULL;
	priv->btn_view               = NULL;
	priv->btn_pref               = NULL;
	priv->btn_show               = NULL;
	priv->status_string          = NULL;
	priv->updater_stopped        = TRUE;

	priv->builder = gtk_builder_new ();
	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);

	priv->watcher_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                         UPDATER_SERVICE_NAME,
                                         G_BUS_NAME_WATCHER_FLAGS_NONE,
                                         name_appeared_cb,
                                         name_vanished_cb,
                                         module, NULL);
}

static void
build_control_ui (UpdaterModule *module)
{
	GError *error = NULL;

	UpdaterModulePrivate *priv = module->priv;

	gtk_builder_add_from_resource (priv->builder, UPDATER_CONTROL_UI, &error);
	if (error) {
		g_warning ("Failed to build updater ui: %s", error->message);
		g_error_free (error);
		return;
	}

	priv->control = GET_WIDGET (priv->builder, "control");
	priv->img_status = GET_WIDGET (priv->builder, "img_status");
	priv->lbl_updater_status = GET_WIDGET (priv->builder, "lbl_updater_status");

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->img_status),
                                  "updater-base-unkown",
                                  GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_pixel_size (GTK_IMAGE (priv->img_status), STATUS_ICON_SIZE);

	gtk_widget_set_sensitive (priv->control, !priv->updater_stopped);

	gtk_widget_show_all (priv->control);
}

static void
build_control_menu_ui (UpdaterModule *module)
{
	GError *error = NULL;
	UpdaterModulePrivate *priv = module->priv;

	gtk_builder_add_from_resource (priv->builder, UPDATER_CONTROL_MENU_UI, &error);
	if (error) {
		g_error_free (error);
		return;
	}

	priv->btn_view          = GET_WIDGET (priv->builder, "btn_view");
	priv->btn_pref          = GET_WIDGET (priv->builder, "btn_pref");
	priv->btn_show          = GET_WIDGET (priv->builder, "btn_show");
	priv->box_updater_menu  = GET_WIDGET (priv->builder, "box_updater_menu");

	g_signal_connect (G_OBJECT (priv->btn_view), "clicked",
                      G_CALLBACK (on_view_button_clicked), module);
	g_signal_connect (G_OBJECT (priv->btn_pref), "clicked",
                      G_CALLBACK (on_pref_button_clicked), module);
	g_signal_connect (G_OBJECT (priv->btn_show), "clicked",
                      G_CALLBACK (on_show_button_clicked), module);

	check_synaptic (module);
}

UpdaterModule *
updater_module_new (void)
{
	return g_object_new (MODULE_TYPE_UPDATER, NULL);
}

GtkWidget *
updater_module_tray_new (UpdaterModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	UpdaterModulePrivate *priv = module->priv;

	if (!priv->tray) {
		priv->tray = gtk_image_new_from_icon_name ("updater-base-unkown", GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
	}

	gtk_widget_show (priv->tray);

	return priv->tray;
}

GtkWidget *
updater_module_control_new (UpdaterModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	UpdaterModulePrivate *priv = module->priv;

	build_control_ui (module);

    g_idle_add ((GSourceFunc)updater_status_update_idle, module);

	return priv->control;
}

GtkWidget *
updater_module_control_menu_new (UpdaterModule *module)

{
	g_return_val_if_fail (module != NULL, NULL);

	UpdaterModulePrivate *priv = module->priv;

	build_control_menu_ui (module);

	gtk_widget_show (priv->box_updater_menu);

	return priv->box_updater_menu;
}

void
updater_module_control_destroy (UpdaterModule *module)
{
	g_return_if_fail (module != NULL);

	UpdaterModulePrivate *priv = module->priv;

	if (priv->control) {
		gtk_widget_destroy (priv->control);
		priv->control = NULL;
	}
}

void
updater_module_control_menu_destroy (UpdaterModule *module)
{
	g_return_if_fail (module != NULL);

	UpdaterModulePrivate *priv = module->priv;

	if (priv->box_updater_menu) {
		gtk_widget_destroy (priv->box_updater_menu);
		priv->box_updater_menu = NULL;
	}
}
