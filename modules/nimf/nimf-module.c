/*
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

#include <stdio.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include "common.h"
#include "nimf-module.h"

#define NIMF_GOOROOM_SERVICE_NAME       "kr.gooroom.nimf.Service"
#define NIMF_GOOROOM_SERVICE_PATH       "/kr/gooroom/nimf/Service"
#define NIMF_GOOROOM_SERVICE_INTERFACE  "kr.gooroom.nimf.Service"

#define NIMF_SETTINGS_DESKTOP			"nimf-settings.desktop"

#define GET_WIDGET(builder, x) GTK_WIDGET (gtk_builder_get_object (builder, x))


struct _NimfModulePrivate
{
	GtkWidget  *tray;
	GtkWidget  *control;
	GtkWidget  *control_menu;

	GtkBuilder *builder;

	GDBusProxy *proxy;

	gchar      *engine_id;
	guint       watcher_id;

	gboolean    nimf_stopped;
};

enum {
	LAUNCH_DESKTOP,
	CHANGE_ENGINE_DONE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (NimfModule, nimf_module, G_TYPE_OBJECT)


static gchar **
get_active_engines (void)
{
	gchar **ret = NULL;
	GSettingsSchema *schema = NULL;
	GSettingsSchemaSource *schema_source = NULL;

	schema_source = g_settings_schema_source_get_default ();

	schema = g_settings_schema_source_lookup (schema_source, "org.nimf", TRUE);
	if (g_settings_schema_has_key (schema, "hidden-active-engines")) {
		GSettings *gsettings = g_settings_new_full (schema, NULL, NULL);
		ret = g_settings_get_strv (gsettings, "hidden-active-engines");
		g_object_unref (gsettings);
	}
	g_settings_schema_unref (schema);

	return ret;
}

static gchar *
get_default_engine_id (void)
{
	gchar *ret = NULL;
	GSettingsSchema *schema = NULL;
	GSettingsSchemaSource *schema_source = NULL;

	schema_source = g_settings_schema_source_get_default ();

	schema = g_settings_schema_source_lookup (schema_source, "org.nimf.engines", TRUE);
	if (g_settings_schema_has_key (schema, "default-engine")) {
		GSettings *gsettings = g_settings_new_full (schema, NULL, NULL);
		ret = g_settings_get_string (gsettings, "default-engine");
		g_object_unref (gsettings);
	}
	g_settings_schema_unref (schema);

	return ret;
}

static gchar *
get_default_engine_name (void)
{
	gchar *ret = NULL;
	gchar *default_engine_id = NULL;
	GSettingsSchemaSource *schema_source = NULL;

	schema_source = g_settings_schema_source_get_default ();

	default_engine_id = get_default_engine_id ();

	if (default_engine_id) {
		gchar *schema_id = g_strdup_printf ("org.nimf.engines.%s", default_engine_id);
		GSettingsSchema *schema = g_settings_schema_source_lookup (schema_source, schema_id, TRUE);
		if (g_settings_schema_has_key (schema, "hidden-schema-name")) {
			GSettings *gsettings = g_settings_new_full (schema, NULL, NULL);
			ret = g_settings_get_string (gsettings, "hidden-schema-name");
			g_object_unref (gsettings);
		}
		g_free (schema_id);
		g_settings_schema_unref (schema);
	}

	g_free (default_engine_id);

	return ret;
}

static gchar *
get_engine_name_by_id (const gchar *engine_id)
{
	g_return_val_if_fail (engine_id != NULL, NULL);

	gchar *ret = NULL;
	GSettingsSchemaSource *schema_source = NULL;

	schema_source = g_settings_schema_source_get_default ();

	gchar *schema_id = g_strdup_printf ("org.nimf.engines.%s", engine_id);
	GSettingsSchema *schema = g_settings_schema_source_lookup (schema_source, schema_id, TRUE);
	if (g_settings_schema_has_key (schema, "hidden-schema-name")) {
		GSettings *gsettings = g_settings_new_full (schema, NULL, NULL);
		ret = g_settings_get_string (gsettings, "hidden-schema-name");
		g_object_unref (gsettings);
	}
	g_free (schema_id);
	g_settings_schema_unref (schema);

	return ret;
}

static void
update_tray (GtkWidget *tray, const gchar *icon_name)
{
	g_return_if_fail (tray != NULL);

	gtk_image_set_from_icon_name (GTK_IMAGE (tray), icon_name, GTK_ICON_SIZE_BUTTON);
	gtk_image_set_pixel_size (GTK_IMAGE (tray), TRAY_ICON_SIZE);
}

static void
get_nimf_status_done_cb (GDBusProxy   *proxy,
                         GAsyncResult *res,
                         gpointer      data)
{
	GVariant *result;
	const gchar *engine_id, *icon_name;
	GError *error = NULL;

	NimfModule *module = NIMF_MODULE (data);
	NimfModulePrivate *priv = module->priv;

	result = g_dbus_proxy_call_finish (proxy, res, &error);

	if (error) {
		g_warning ("Failed to get nimf engine id : %s\n", error->message);
		g_error_free (error);
		priv->nimf_stopped = TRUE;
		icon_name = "gpm-brightness-kbd-invalid";
	} else {
		priv->nimf_stopped = FALSE;
		if (g_variant_is_of_type (result, G_VARIANT_TYPE ("(ss)"))) {
			g_variant_get (result, "(&s&s)", &engine_id, &icon_name);

			if (!g_str_equal (engine_id, "")) {
				g_free (priv->engine_id);
				priv->engine_id = g_strdup (engine_id);
			}
		}
	}

	update_tray (priv->tray, icon_name);

	g_variant_unref (result);
}

static void
nimf_service_signal_cb (GDBusProxy  *proxy,
                        const gchar *sender_name,
                        const gchar *signal_name,
                        GVariant    *parameters,
                        gpointer     data)
{
	NimfModule *module = NIMF_MODULE (data);
	NimfModulePrivate *priv = module->priv;

	if (g_str_equal (signal_name, "EngineChanged")) {
		const gchar *engine_id, *icon_name;
		g_variant_get (parameters, "(&s&s)", &engine_id, &icon_name);

		if (!g_str_equal (engine_id, "")) {
			g_free (priv->engine_id);
			priv->engine_id = g_strdup (engine_id);
		}

		update_tray (priv->tray, icon_name);

	} else if (g_str_equal (signal_name, "EngineStatusChanged")) {
		const gchar *engine_id, *icon_name;
		g_variant_get (parameters, "(&s&s)", &engine_id, &icon_name);

		update_tray (priv->tray, icon_name);
	} else if (g_str_equal (signal_name, "Stopped")) {
		priv->nimf_stopped = TRUE;
		update_tray (priv->tray, "gpm-brightness-kbd-invalid");
	} else {
	}
}

static void
name_appeared_cb (GDBusConnection *connection,
                  const gchar     *name,
                  const gchar     *name_owner,
                  gpointer         data)
{
	GError *error;

	NimfModule *module = NIMF_MODULE (data);
	NimfModulePrivate *priv = module->priv;

	priv->nimf_stopped = FALSE;

	priv->proxy = g_dbus_proxy_new_sync (connection,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         NIMF_GOOROOM_SERVICE_NAME,
                                         NIMF_GOOROOM_SERVICE_PATH,
                                         NIMF_GOOROOM_SERVICE_INTERFACE,
                                         NULL, &error);

	if (error) {
		g_warning ("Failed to get gooroom nimf service proxy: %s\n", error->message);
		g_error_free (error);
		priv->nimf_stopped = TRUE;
		update_tray (priv->tray, "gpm-brightness-kbd-invalid");
		return;
	}

	g_dbus_proxy_call (priv->proxy,
                       "GetStatus",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       (GAsyncReadyCallback) get_nimf_status_done_cb,
                       module);

	g_signal_connect (priv->proxy, "g-signal", G_CALLBACK (nimf_service_signal_cb), module);
}

static void
name_vanished_cb (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         data)
{
	NimfModule *module = NIMF_MODULE (data);
	NimfModulePrivate *priv = module->priv;

	priv->nimf_stopped = TRUE;

	g_free (priv->engine_id);
	priv->engine_id = NULL;

	update_tray (priv->tray, "gpm-brightness-kbd-invalid");
}

static void
on_engine_button_clicked (GtkButton *button, gpointer data)
{
	GVariant *result;
	NimfModule *module = NIMF_MODULE (data);
	NimfModulePrivate *priv = module->priv;

	gchar *method_id = g_strdup ("");
	gchar *engine_id =  g_object_get_data (G_OBJECT (button), "engine-id");

	if (engine_id) {
		gchar *schema_id = g_strdup_printf ("org.nimf.engines.%s", engine_id);

		GSettingsSchemaSource *schema_source = g_settings_schema_source_get_default ();
		GSettingsSchema *schema = g_settings_schema_source_lookup (schema_source, schema_id, TRUE);
		if (g_settings_schema_has_key (schema, "get-method-infos")) {
			GSettings *gsettings = g_settings_new (schema_id);
			gchar *method_infos = g_settings_get_string (gsettings, "get-method-infos");
			if (method_infos) {
				g_free (method_id);
				method_id = g_strdup (method_infos);
			}
			g_free (method_infos);
			g_object_unref (gsettings);
		}
		g_settings_schema_unref (schema);

		result = g_dbus_proxy_call_sync (priv->proxy,
                                         "ChangeEngine",
                                         g_variant_new ("(ss)", engine_id, method_id),
                                         G_DBUS_CALL_FLAGS_NONE,
                                         -1,
                                         NULL,
                                         NULL);

		if (g_variant_is_of_type (result, G_VARIANT_TYPE ("(b)"))) {
			gboolean ret = FALSE;
			g_variant_get (result, "(b)", &ret);
			if (ret) {
			}
		}

		g_variant_unref (result);

		g_signal_emit (G_OBJECT (module), signals[CHANGE_ENGINE_DONE], 0);
	}

	g_free (method_id);
}

static void
on_settings_button_clicked (GtkButton *button, gpointer data)
{
	NimfModule *module = NIMF_MODULE (data);
	NimfModulePrivate *priv = module->priv;

	g_signal_emit (G_OBJECT (module), signals[LAUNCH_DESKTOP], 0, NIMF_SETTINGS_DESKTOP);
}

static GtkWidget *
menu_button_new (const gchar *text, gboolean checked)
{
	GtkWidget *button, *box, *image, *label;

	button = gtk_button_new ();
	gtk_widget_set_name (button, "module-widget");
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_container_add (GTK_CONTAINER (button), box);
	gtk_widget_show (box);

	if (checked) {
		image = gtk_image_new_from_icon_name ("object-select-symbolic", GTK_ICON_SIZE_BUTTON);
		gtk_widget_set_name (image, "nimf-selected");
	} else {
		image = gtk_image_new_from_icon_name ("", GTK_ICON_SIZE_BUTTON);
	}
	gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 0);
	gtk_widget_show (image);

	label = gtk_label_new (text);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	return button;
}

static void
build_control_ui (NimfModule *module, GtkSizeGroup *size_group)
{
	GError *error = NULL;
	gchar *engine_name = NULL;
	GtkWidget *lbl_engine_id, *img_icon_name;
	NimfModulePrivate *priv = module->priv;

	gtk_builder_add_from_resource (priv->builder,"/kr/gooroom/IntegrationApplet/modules/nimf/nimf-control.ui", &error);
	if (error) {
		g_error_free (error);
		return;
	}

	priv->control = GET_WIDGET (priv->builder, "control");
	lbl_engine_id = GET_WIDGET (priv->builder, "lbl_engine_id");
	img_icon_name = GET_WIDGET (priv->builder, "img_icon_name");

	gtk_image_set_from_icon_name (GTK_IMAGE (img_icon_name),
                                  "nimf-system-keyboard", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_pixel_size (GTK_IMAGE (img_icon_name), STATUS_ICON_SIZE);

	if (priv->nimf_stopped) {
		engine_name = g_strdup (_("Input Method Not Running"));
		gtk_widget_set_sensitive (priv->control, FALSE);
	} else {
		if (priv->engine_id) {
			engine_name = get_engine_name_by_id (priv->engine_id);
		} else {
			engine_name = get_default_engine_name ();
		}
	}

	gchar *markup = g_markup_printf_escaped ("<b>%s</b>", engine_name);
	gtk_label_set_markup (GTK_LABEL (lbl_engine_id), markup);

	g_free (markup);
	g_free (engine_name);

	if (size_group)
		gtk_size_group_add_widget (size_group, img_icon_name);

	gtk_widget_show_all (priv->control);
}

static void
build_control_menu_ui (NimfModule *module)
{
	guint i = 0;
	gchar **engines = NULL;
	gchar *curr_engine_id = NULL;
	GSettingsSchemaSource *schema_source = NULL;
	NimfModulePrivate *priv = module->priv;

	schema_source = g_settings_schema_source_get_default ();

	priv->control_menu = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (priv->control_menu),
                                    GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (priv->control_menu), vbox);
	gtk_widget_show (vbox);

	curr_engine_id = (priv->engine_id) ? g_strdup (priv->engine_id) : get_default_engine_id ();

	engines = get_active_engines ();
	for (i = 0; engines[i] != NULL; i++) {
		gboolean checked = g_str_equal (engines[i], curr_engine_id);
		gchar *schema_id = g_strdup_printf ("org.nimf.engines.%s", engines[i]);

		GSettingsSchema *schema = g_settings_schema_source_lookup (schema_source, schema_id, TRUE);
		if (g_settings_schema_has_key (schema, "hidden-schema-name")) {
			GSettings *gsettings = g_settings_new_full (schema, NULL, NULL);
			gchar *engine_name = g_settings_get_string (gsettings, "hidden-schema-name");
			g_object_unref (gsettings);

			if (engine_name) {
				GtkWidget *button = menu_button_new (engine_name, checked);
				g_object_set_data_full (G_OBJECT (button), "engine-id", g_strdup (engines[i]), g_free);
				gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
				gtk_widget_set_name (button, "nimf-module-widget");
				gtk_widget_show_all (button);

				g_signal_connect (G_OBJECT (button), "clicked",
                                  G_CALLBACK (on_engine_button_clicked), module);
			}
			g_free (engine_name);
		}
		g_settings_schema_unref (schema);
		g_free (schema_id);
	}

	g_free (curr_engine_id);
	g_strfreev (engines);

	GtkWidget *button = menu_button_new (_("Input Method Settings"), FALSE);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_widget_set_name (button, "nimf-module-widget");
	gtk_widget_show_all (button);

	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (on_settings_button_clicked), module);
}

static void
nimf_module_finalize (GObject *object)
{
	NimfModule *module = NIMF_MODULE (object);
	NimfModulePrivate *priv = module->priv;

	if (priv->watcher_id) {
		g_bus_unwatch_name (priv->watcher_id);
		priv->watcher_id = 0;
	}

	g_clear_object (&priv->proxy);
	g_clear_object (&priv->builder);

	G_OBJECT_CLASS (nimf_module_parent_class)->finalize (object);
}

static void
nimf_module_class_init (NimfModuleClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = nimf_module_finalize;

	signals[LAUNCH_DESKTOP] = g_signal_new ("launch-desktop",
                                            MODULE_TYPE_NIMF,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(NimfModuleClass,
                                            launch_desktop),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);

	signals[CHANGE_ENGINE_DONE] = g_signal_new ("change-engine-done",
                                                MODULE_TYPE_NIMF,
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET(NimfModuleClass,
                                                change_engine_done),
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE, 0);
}

static void
nimf_module_init (NimfModule *module)
{
	GError *error = NULL;
	NimfModulePrivate *priv;

	module->priv = priv = nimf_module_get_instance_private (module);

	priv->tray          = NULL;
	priv->engine_id     = NULL;
	priv->nimf_stopped  = FALSE;

	priv->builder = gtk_builder_new ();
	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);

	priv->watcher_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                         NIMF_GOOROOM_SERVICE_NAME,
                                         G_BUS_NAME_WATCHER_FLAGS_NONE,
                                         name_appeared_cb,
                                         name_vanished_cb,
                                         module, NULL);
}

NimfModule *
nimf_module_new (void)
{
	return g_object_new (MODULE_TYPE_NIMF, NULL);
}

GtkWidget *
nimf_module_tray_new (NimfModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	NimfModulePrivate *priv = module->priv;

	if (!priv->tray) {
		priv->tray = gtk_image_new_from_icon_name ("nimf-focus-out", GTK_ICON_SIZE_BUTTON);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
	}

	gtk_widget_show (priv->tray);

	return priv->tray;
}

GtkWidget *
nimf_module_control_new (NimfModule *module, GtkSizeGroup *size_group)
{
	g_return_val_if_fail (module != NULL, NULL);

	build_control_ui (module, size_group);

	return module->priv->control;
}

GtkWidget *
nimf_module_control_menu_new (NimfModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	NimfModulePrivate *priv = module->priv;

	build_control_menu_ui (module);

	gtk_widget_show (priv->control_menu);

	return priv->control_menu;
}

void
nimf_module_control_destroy (NimfModule *module)
{
	g_return_if_fail (module != NULL);

	NimfModulePrivate *priv = module->priv;

	if (priv->control) {
		gtk_widget_destroy (priv->control);
		priv->control = NULL;
	}
}

void
nimf_module_control_menu_destroy (NimfModule *module)
{
	g_return_if_fail (module != NULL);

	NimfModulePrivate *priv = module->priv;

	if (priv->control_menu) {
		gtk_widget_destroy (priv->control_menu);
		priv->control_menu = NULL;
	}
}
