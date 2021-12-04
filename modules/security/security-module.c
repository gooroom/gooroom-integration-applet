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

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include <json-c/json.h>

#include <libnotify/notify.h>

#include "utils.h"
#include "common.h"
#include "security-module.h"


#define SECURITY_STATUS_UPDATE_TIMEOUT      5000

#define SECURITY_STATUS_TOOL_DESKTOP        "gooroom-security-status-tool.desktop"

#define GET_WIDGET(builder, x) GTK_WIDGET (gtk_builder_get_object (builder, x))


#define SECURITY_ITEM_OS_VULNERABLE     (1 << 0)
#define SECURITY_ITEM_EXE_VULNERABLE    (1 << 1)
#define SECURITY_ITEM_BOOT_VULNERABLE   (1 << 2)
#define SECURITY_ITEM_MEDIA_VULNERABLE  (1 << 3)



struct _SecurityModulePrivate
{
	GtkWidget *tray;
	GtkWidget *control;
	GtkWidget *lbl_sec_status;
	GtkWidget *img_status;
	GtkWidget *btn_sec_more;
	GtkWidget *box_sec_menu;
	GtkWidget *btn_sec_safety;

	GSettings *settings;

	GtkBuilder *builder;

	guint timeout_id;
	GPid log_parser_pid;

	gboolean notification_show;
	gboolean updating_sec_status;

	NotifyNotification *notification;
};

enum {
	LAUNCH_DESKTOP,
	STATUS_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (SecurityModule, security_module, G_TYPE_OBJECT)

static void
notify_notification (NotifyNotification *notification, guint vulnerable)
{
	GdkPixbuf *pix = NULL;
	gchar *full_body = NULL;

	const gchar *summary = _("Security Status Of Gooroom System");
	const gchar *body = _("A security vulnerability has been detected.");

	pix = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                    "security-status-vulnerable",
                                    48, GTK_ICON_LOOKUP_FORCE_SIZE,
                                    NULL);

	GString *outputs = g_string_new ("");

	if (vulnerable & SECURITY_ITEM_OS_VULNERABLE) {
		if (outputs->len > 0) g_string_append_c (outputs, ',');
		g_string_append_printf (outputs, _("Protecting OS"));
	}
	if (vulnerable & SECURITY_ITEM_EXE_VULNERABLE) {
		if (outputs->len > 0) g_string_append_c (outputs, ',');
		g_string_append (outputs, _("Protect executable files"));
	}
	if (vulnerable & SECURITY_ITEM_BOOT_VULNERABLE) {
		if (outputs->len > 0) g_string_append_c (outputs, ',');
		g_string_append (outputs, _("Trusted Booting"));
	}
	if (vulnerable & SECURITY_ITEM_MEDIA_VULNERABLE) {
		if (outputs->len > 0) g_string_append_c (outputs, ',');
		g_string_append_printf (outputs, _("Resources Control"));
	}

	if (outputs->len > 0) {
		full_body = g_strdup_printf ("[%s] %s", outputs->str, body);
	} else {
		full_body = g_strdup (body);
	}

	g_string_free (outputs, TRUE);

	notify_notification_update (notification, summary, full_body, NULL);
	notify_notification_set_image_from_pixbuf (notification, pix);
	notify_notification_show (notification, NULL);

	g_free (full_body);
}

static guint
get_last_vulnerable (void)
{
	guint vulnerable = 0;
	gchar *str_vulnerable = NULL;

	if (g_file_test (GOOROOM_SECURITY_STATUS_VULNERABLE, G_FILE_TEST_EXISTS)) {
		g_file_get_contents (GOOROOM_SECURITY_STATUS_VULNERABLE, &str_vulnerable, NULL, NULL);

		vulnerable = atoi (str_vulnerable);

		//if (1 == sscanf (str_vulnerable, "%"G_GUINT32_FORMAT, &vulnerable)) {
		if (vulnerable) {
			if ((vulnerable < (1 << 0)) || (vulnerable >= (1 << 4))) { // 1 <= vulnerable < 16
				vulnerable = 0;
			}
		}
	}

	g_free (str_vulnerable);

	return vulnerable;
}


static void
last_vulnerable_update (guint vulnerable)
{
	gchar *pkexec = NULL, *cmd = NULL;

	pkexec = g_find_program_in_path ("pkexec");
	cmd = g_strdup_printf ("%s %s %u", pkexec, GOOROOM_SECURITY_STATUS_VULNERABLE_HELPER, vulnerable);

	g_spawn_command_line_sync (cmd, NULL, NULL, NULL, NULL);
}

static void
security_log_parser_done_cb (GPid pid, gint status, gpointer data)
{
	SecurityModule *module = SECURITY_MODULE (data);

	g_spawn_close_pid (pid);

	module->priv->log_parser_pid = -1;
}

static gboolean
read_log_parser_result (GIOChannel   *source,
                        GIOCondition  condition,
                        gpointer      data)
{
	gchar  buff[1024] = {0, };
	gsize  bytes_read;
	gchar *markup = NULL;
	gboolean sensitive = FALSE;
	const gchar *tray, *icon;
	guint vulnerable = 0;

	SecurityModule *module = SECURITY_MODULE (data);
	SecurityModulePrivate *priv = module->priv;

	GString *outputs = g_string_new ("");

	while (g_io_channel_read_chars (source, buff, sizeof (buff), &bytes_read, NULL) == G_IO_STATUS_NORMAL) {
		outputs = g_string_append_len (outputs, buff, bytes_read);
	}

	if (!outputs->str || outputs->len <= 0) {
		icon = "security-status-unknown";
		if (priv->control) {
			markup = g_markup_printf_escaped ("<b><i><span>%s</span></i></b>", _("Unknown"));
		}

		if (priv->notification) {
			notify_notification_close (priv->notification, NULL);
			priv->notification_show = FALSE;
		}

		goto error;
	}

	gchar *json_output = strstr (outputs->str, "JSON-ANCHOR=");
	if (json_output) {
		gchar *json_string = json_output + strlen ("JSON-ANCHOR=");
		enum json_tokener_error jerr = json_tokener_success;
		json_object *root_obj = json_tokener_parse_verbose (json_string, &jerr);
		if (jerr == json_tokener_success) {
			json_object *os_status_obj, *exe_status_obj, *boot_status_obj, *media_status_obj;

			os_status_obj = JSON_OBJECT_GET (root_obj, "os_status");
			exe_status_obj = JSON_OBJECT_GET (root_obj, "exe_status");
			boot_status_obj = JSON_OBJECT_GET (root_obj, "boot_status");
			media_status_obj = JSON_OBJECT_GET (root_obj, "media_status");

			if (os_status_obj) {
				const char *val = json_object_get_string (os_status_obj);
				if (val) {
					if (g_strcmp0 (val, "vulnerable") == 0) {
						vulnerable |= SECURITY_ITEM_OS_VULNERABLE;
					}
				}
			}
			if (exe_status_obj) {
				const char *val = json_object_get_string (exe_status_obj);
				if (val) {
					if (g_strcmp0 (val, "vulnerable") == 0) {
						vulnerable |= SECURITY_ITEM_EXE_VULNERABLE;
					}
				}
			}
			if (boot_status_obj) {
				const char *val = json_object_get_string (boot_status_obj);
				if (val) {
					if (g_strcmp0 (val, "vulnerable") == 0) {
						vulnerable |= SECURITY_ITEM_BOOT_VULNERABLE;
					}
				}
			}
			if (media_status_obj) {
				const char *val = json_object_get_string (media_status_obj);
				if (val) {
					if (g_strcmp0 (val, "vulnerable") == 0) {
						vulnerable |= SECURITY_ITEM_MEDIA_VULNERABLE;
					}
				}
			}
			json_object_put (root_obj);
		}
	}

	guint last_vulnerable = get_last_vulnerable ();

	if (vulnerable == 0 && last_vulnerable == 0) {
		icon = "security-status-safety";
		if (priv->control) {
			markup = g_markup_printf_escaped ("<b><i><span foreground=\"#7ED321\">%s</span></i></b>", _("Safety"));
		}
		if (priv->notification) {
			notify_notification_close (priv->notification, NULL);
			priv->notification_show = FALSE;
		}
	} else {
		if (vulnerable != 0)
			last_vulnerable_update (vulnerable);

		icon = "security-status-vulnerable";
		if (priv->control) {
			sensitive = TRUE;
			markup = g_markup_printf_escaped ("<b><i><span foreground=\"#ff0000\">%s</span></i></b>", _("Vulnerable"));
		}
		if (priv->notification) {
			if (priv->notification_show) {
				gboolean adn = FALSE;
				if (priv->settings) {
					adn = g_settings_get_boolean (priv->settings, "allow-duplicate-notifications");
				}
				if (adn) {
					notify_notification (priv->notification, vulnerable);
				}
			} else {
				notify_notification (priv->notification, vulnerable);
				priv->notification_show = TRUE;
			}
		}
	}

error:
	g_string_free (outputs, TRUE);

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->tray), icon, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);

	if (priv->control) {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->img_status), icon, GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->img_status), STATUS_ICON_SIZE);

		gtk_label_set_markup (GTK_LABEL (priv->lbl_sec_status), markup);

		g_signal_emit (G_OBJECT (module), signals[STATUS_CHANGED], 0, markup);

		g_free (markup);
	}

	if (priv->box_sec_menu) {
		if (gtk_widget_get_visible (priv->btn_sec_safety))
			gtk_widget_set_sensitive (priv->btn_sec_safety, sensitive);
	}

	priv->updating_sec_status = FALSE;

	return FALSE;
}

static gboolean
security_status_update_idle (gpointer data)
{
	SecurityModule *module = SECURITY_MODULE (data);
	SecurityModulePrivate *priv = module->priv;

	if (priv->updating_sec_status)
		return TRUE;

	priv->updating_sec_status = TRUE;

	if (priv->log_parser_pid > 0)
		kill (priv->log_parser_pid, SIGTERM);

	if (!run_security_log_parser_async (&priv->log_parser_pid, read_log_parser_result, data)) {
		gtk_image_set_from_icon_name (GTK_IMAGE (priv->tray),
                                      "security-status-unknown",
                                      GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);

		if (priv->control) {
			gchar *markup = g_markup_printf_escaped ("%s", _("Unknown"));

			gtk_image_set_from_icon_name (GTK_IMAGE (priv->img_status),
                                          "security-status-unknown", GTK_ICON_SIZE_LARGE_TOOLBAR);
			gtk_image_set_pixel_size (GTK_IMAGE (priv->img_status), STATUS_ICON_SIZE);

			gtk_label_set_markup (GTK_LABEL (priv->lbl_sec_status), markup);

			g_signal_emit (G_OBJECT (module), signals[STATUS_CHANGED], 0, markup);

			g_free (markup);
		}

		if (priv->box_sec_menu) {
			if (gtk_widget_get_visible (priv->btn_sec_safety))
				gtk_widget_set_sensitive (priv->btn_sec_safety, FALSE);
		}

		if (priv->notification) {
			notify_notification_close (priv->notification, NULL);
			priv->notification_show = FALSE;
		}

		priv->updating_sec_status = FALSE;
	}

	return TRUE;
}

static gboolean
security_status_update_continually_idle (gpointer data)
{
	SecurityModule *module = SECURITY_MODULE (data);
	SecurityModulePrivate *priv = module->priv;

	guint timeout = SECURITY_STATUS_UPDATE_TIMEOUT;

	if (priv->settings)
		timeout = g_settings_get_uint (priv->settings, "cycle-time");

	if (priv->timeout_id != 0) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	if (timeout == 0)
		return FALSE;

	if (timeout < SECURITY_STATUS_UPDATE_TIMEOUT)
		timeout = SECURITY_STATUS_UPDATE_TIMEOUT;

	security_status_update_idle (module);

	priv->timeout_id = g_timeout_add (timeout, (GSourceFunc) security_status_update_idle, module);

	return FALSE;
}

static void
file_status_changed_cb (GFileMonitor      *monitor,
                        GFile             *file,
                        GFile             *other_file,
                        GFileMonitorEvent  event_type,
                        gpointer           data)
{
	SecurityModule *module = SECURITY_MODULE (data);
	SecurityModulePrivate *priv = module->priv;

	switch (event_type)
	{
		case G_FILE_MONITOR_EVENT_CHANGED:
		case G_FILE_MONITOR_EVENT_DELETED:
		case G_FILE_MONITOR_EVENT_CREATED:
		{
			g_timeout_add (100, (GSourceFunc) security_status_update_continually_idle, module);
			break;
		}

		default:
			break;
	}
}

static void
settings_changed_cb (GSettings   *settings,
                     const gchar *key,
                     gpointer     data)
{
	SecurityModule *module = SECURITY_MODULE (data);
	SecurityModulePrivate *priv = module->priv;

	if (g_str_equal (key, "cycle-time")) {
		g_timeout_add (100, (GSourceFunc) security_status_update_continually_idle, module);
	}
}

static void
on_security_detail_activate (GtkMenuItem *menu_item, gpointer data)
{
	SecurityModule *module = SECURITY_MODULE (data);

	g_signal_emit (G_OBJECT (module), signals[LAUNCH_DESKTOP], 0, SECURITY_STATUS_TOOL_DESKTOP);
}

static void
show_detail_cb (NotifyNotification *n, gchar *action, gpointer data)
{
	SecurityModule *module = SECURITY_MODULE (data);

	on_security_detail_activate (NULL, data);
}

static void
on_more_button_clicked (GtkButton *button, gpointer data)
{
	on_security_detail_activate (NULL, data);
}

static void
on_safety_measure_button_clicked (GtkButton *button, gpointer data)
{
	SecurityModule *module = SECURITY_MODULE (data);
	SecurityModulePrivate *priv = module->priv;

	if (gtk_widget_get_visible (priv->btn_sec_safety))
		gtk_widget_set_sensitive (priv->btn_sec_safety, FALSE);

	if (is_systemd_service_active (GOOROOM_AGENT_SERVICE_NAME)) {
		send_taking_measures_signal_to_agent ();
	} else {
		send_taking_measure_signal_to_self ();
	}

	last_vulnerable_update (0);
}

static void
build_control_ui (SecurityModule *module)
{
	GError *error = NULL;
	SecurityModulePrivate *priv = module->priv;

	gtk_builder_add_from_resource (priv->builder,
                                   "/kr/gooroom/IntegrationApplet/modules/security/security-control.ui",
                                   &error);
	if (error) {
		g_error_free (error);
		return;
	}

	priv->control = GET_WIDGET (priv->builder, "control");
	priv->img_status = GET_WIDGET (priv->builder, "img_status");
	priv->lbl_sec_status = GET_WIDGET (priv->builder, "lbl_sec_status");

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->img_status),
                                  "security-status-unknown",
                                  GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_pixel_size (GTK_IMAGE (priv->img_status), STATUS_ICON_SIZE);

	gtk_widget_show_all (priv->control);
}

static void
build_control_menu_ui (SecurityModule *module)
{
	GError *error = NULL;
	SecurityModulePrivate *priv = module->priv;

	gtk_builder_add_from_resource (priv->builder, "/kr/gooroom/IntegrationApplet/modules/security/security-control-menu.ui", &error);
	if (error) {
		g_error_free (error);
		return;
	}

	priv->box_sec_menu = GET_WIDGET (priv->builder, "box_sec_menu");
	priv->btn_sec_more = GET_WIDGET (priv->builder, "btn_sec_more");
	priv->btn_sec_safety = GET_WIDGET (priv->builder, "btn_sec_safety");

	g_signal_connect (G_OBJECT (priv->btn_sec_more), "clicked", G_CALLBACK (on_more_button_clicked), module);
	g_signal_connect (G_OBJECT (priv->btn_sec_safety), "clicked", G_CALLBACK (on_safety_measure_button_clicked), module);

	if (is_admin_group () && is_local_user ()) {
		gtk_widget_show (priv->btn_sec_safety);
		gtk_widget_set_sensitive (priv->btn_sec_safety, FALSE);
	} else {
		gtk_widget_hide (priv->btn_sec_safety);
	}
}

static void
security_module_finalize (GObject *object)
{
	SecurityModule *module = SECURITY_MODULE (object);
	SecurityModulePrivate *priv = module->priv;

	g_clear_object (&priv->builder);
	g_clear_object (&priv->settings);

	if (priv->timeout_id != 0) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	if (priv->log_parser_pid > 0)
		kill (priv->log_parser_pid, SIGTERM);

	if (priv->notification) {
		notify_notification_close (priv->notification, NULL);
		g_object_unref (priv->notification);
		priv->notification = NULL;
	}

	G_OBJECT_CLASS (security_module_parent_class)->finalize (object);
}

static void
security_module_class_init (SecurityModuleClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = security_module_finalize;

	signals[LAUNCH_DESKTOP] = g_signal_new ("launch-desktop",
                                            MODULE_TYPE_SECURITY,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(SecurityModuleClass,
                                            launch_desktop),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);

	signals[STATUS_CHANGED] = g_signal_new ("status-changed",
                                            MODULE_TYPE_SECURITY,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(SecurityModuleClass,
                                            status_changed),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);
}

static void
security_module_init (SecurityModule *module)
{
	GFile *file;
	GError *error = NULL;
	GSettingsSchema *schema;
	GFileMonitor *monitor;
	SecurityModulePrivate *priv;

	module->priv = priv = security_module_get_instance_private (module);

	priv->tray           = NULL;
	priv->control        = NULL;
	priv->box_sec_menu   = NULL;
	priv->lbl_sec_status = NULL;
	priv->img_status     = NULL;
	priv->notification   = NULL;
	priv->settings       = NULL;
	priv->timeout_id     = 0;
	priv->log_parser_pid = -1;
	priv->notification_show = FALSE;
	priv->updating_sec_status = FALSE;

	priv->builder = gtk_builder_new ();
	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);

	schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (), "apps.gooroom-security-status", TRUE);
	if (schema) {
		priv->settings = g_settings_new_full (schema, NULL, NULL);
		g_signal_connect (priv->settings, "changed",
                          G_CALLBACK (settings_changed_cb), module);

		g_settings_schema_unref (schema);
	}

	priv->notification = notify_notification_new (NULL, NULL, NULL);
	notify_notification_set_timeout (priv->notification, NOTIFY_EXPIRES_NEVER);
	notify_notification_add_action (priv->notification, "details-action", _("Details"), show_detail_cb, module, NULL);

	file = g_file_new_for_path (GOOROOM_SECURITY_STATUS_VULNERABLE);

	error = NULL;
	monitor = g_file_monitor_file (file, G_FILE_MONITOR_NONE, NULL, &error);
	if (error) {
		g_error_free (error);
	} else {
		g_signal_connect (monitor, "changed", G_CALLBACK (file_status_changed_cb), module);
	}
	g_object_unref (file);
}

SecurityModule *
security_module_new (void)
{
	return g_object_new (MODULE_TYPE_SECURITY, NULL);
}

GtkWidget *
security_module_tray_new (SecurityModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	SecurityModulePrivate *priv = module->priv;

	if (!priv->tray) {
		priv->tray = gtk_image_new_from_icon_name ("security-status-unknown",
                                                   GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
	}

	gtk_widget_show (priv->tray);

	g_timeout_add (3000, (GSourceFunc) security_status_update_continually_idle, module);

	return priv->tray;
}

GtkWidget *
security_module_control_new (SecurityModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	SecurityModulePrivate *priv = module->priv;

	build_control_ui (module);

	g_timeout_add (100, (GSourceFunc) security_status_update_continually_idle, module);


	return priv->control;
}

GtkWidget *
security_module_control_menu_new (SecurityModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	SecurityModulePrivate *priv = module->priv;

	build_control_menu_ui (module);

	g_timeout_add (100, (GSourceFunc) security_status_update_continually_idle, module);

	gtk_widget_show (priv->box_sec_menu);

	return priv->box_sec_menu;
}

void
security_module_control_destroy (SecurityModule *module)
{
	g_return_if_fail (module != NULL);

	SecurityModulePrivate *priv = module->priv;

	if (priv->control) {
		gtk_widget_destroy (priv->control);
		priv->control = NULL;
	}
}

void
security_module_control_menu_destroy (SecurityModule *module)
{
	g_return_if_fail (module != NULL);

	SecurityModulePrivate *priv = module->priv;

	if (priv->box_sec_menu) {
		gtk_widget_destroy (priv->box_sec_menu);
		priv->box_sec_menu = NULL;
	}
}

const gchar *
security_module_get_security_status (SecurityModule *module)
{
	return gtk_label_get_text (GTK_LABEL (module->priv->lbl_sec_status));
}
