/*
 *  Copyright (C) 2023 Gooroom <gooroom@gooroom.kr>
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
#include <config.h>
#endif

#include <stdio.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "common.h"
#include "tablet-module.h"


#define GET_WIDGET(builder, x) GTK_WIDGET (gtk_builder_get_object (builder, x))



struct _TabletModulePrivate
{
	GtkWidget *control;
	GtkWidget *sw_tablet;
	GtkWidget *icon_tablet;

	GtkBuilder *builder;

	gboolean init_tablet_mode;
};

enum {
	LAUNCH_COMMAND,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (TabletModule, tablet_module, G_TYPE_OBJECT)

static gboolean tablet_mode_changed_cb (GtkSwitch *button, gboolean state, gpointer user_data);




static gboolean
launch_tablet_mode_switching_command (gboolean on)
{
	gboolean ret;
	GError *error = NULL;
	gchar *pkexec, *cmdline;

	pkexec = g_find_program_in_path ("pkexec");

	if (on) {
		cmdline = g_strdup_printf ("%s %s", pkexec, TABLET_MODE_CHANGE_HELPER);
	} else {
		cmdline = g_strdup_printf ("%s %s -d", pkexec, TABLET_MODE_CHANGE_HELPER);
	}

	ret = g_spawn_command_line_sync (cmdline, NULL, NULL, NULL, &error);
	if (!ret) {
		if (error) {
			g_warning ("Error attempting to execute command: %s: %s", cmdline, error->message);
			g_error_free (error);
		} else {
			g_warning ("Error attempting to execute command: %s", cmdline);
		}
	}

	g_free (cmdline);

	return ret;
}

static gboolean
logout_idle_cb (gpointer user_data)
{
	gchar *logout_command = NULL, *cmdline = NULL;
	TabletModule *module = TABLET_MODULE (user_data);
	TabletModulePrivate *priv = module->priv;

	logout_command = g_find_program_in_path ("gooroom-logout-command");
	if (logout_command) {
		cmdline = g_strdup_printf ("%s --logout --delay=500", logout_command);
	} else {
		logout_command = g_find_program_in_path ("gnome-session-quit");
		if (logout_command) {
			cmdline = g_strdup_printf ("%s --logout --force --no-prompt", logout_command);
		}
	}

	if (cmdline) {
		g_signal_emit (G_OBJECT (module), signals[LAUNCH_COMMAND], 0, cmdline);
		g_free (cmdline);
	} else {
		const gchar *msg;
		GtkWidget *dialog;

		msg = _("Not found logout command.\n"
                "Install gooroom-logout or gnome-session-bin packages.");

		dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_CLOSE,
                                         NULL);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", msg);
		gtk_window_set_title (GTK_WINDOW (dialog), _("System Logout Error"));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (launch_tablet_mode_switching_command (priv->init_tablet_mode)) {
			g_signal_handlers_block_by_func (priv->sw_tablet, tablet_mode_changed_cb, module);
			gtk_switch_set_active (GTK_SWITCH (priv->sw_tablet), priv->init_tablet_mode);
			g_signal_handlers_unblock_by_func (priv->sw_tablet, tablet_mode_changed_cb, module);
		}
	}

	return FALSE;
}

static void
tablet_mode_switching_dialog_response_cb (GtkDialog *dialog,
                                          gint       response_id,
                                          gpointer   user_data)
{
	GError *error = NULL;
	gchar *cmdline = NULL, *pkexec = NULL;
	TabletModule *module = TABLET_MODULE (user_data);
	TabletModulePrivate *priv = module->priv;

	if (response_id == GTK_RESPONSE_YES) {
		g_idle_add ((GSourceFunc) logout_idle_cb, module);
		goto done;
	}

	if (launch_tablet_mode_switching_command (priv->init_tablet_mode)) {
		g_signal_handlers_block_by_func (priv->sw_tablet, tablet_mode_changed_cb, module);
		gtk_switch_set_active (GTK_SWITCH (priv->sw_tablet), priv->init_tablet_mode);
		g_signal_handlers_unblock_by_func (priv->sw_tablet, tablet_mode_changed_cb, module);
	}

done:
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static gboolean
tablet_mode_changed_cb (GtkSwitch *button, gboolean state, gpointer user_data)
{
	TabletModule *module = TABLET_MODULE (user_data);

	if (state == module->priv->init_tablet_mode)
		return FALSE;

	if (launch_tablet_mode_switching_command (state)) {
		gchar *msg = NULL;
		const gchar *title;
		GtkWidget *dialog;

		if (state) {
			title = _("Switching Tablet Mode");
			msg = _("To switch to tablet mode, you must log in again.\n"
                    "Would you like to log in again now?");
		} else {
			title = _("Switching Normal Mode");
			msg = _("To switch to normal mode, you must log in again.\n"
                    "Would you like to log in again now?");
		}

		dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_NONE,
                                         NULL);
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                                _("Yes"), GTK_RESPONSE_YES,
                                _("No"), GTK_RESPONSE_NO,
                                NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", msg);
		gtk_window_set_title (GTK_WINDOW (dialog), title);
		gtk_widget_show_all (dialog);

		g_signal_connect (dialog, "response",
                          G_CALLBACK (tablet_mode_switching_dialog_response_cb), module);
	}

	return FALSE;
}

static void
build_control_ui (TabletModule *module, GtkSizeGroup *size_group)
{
	GError *error = NULL;
	TabletModulePrivate *priv = module->priv;

	gtk_builder_add_from_resource (priv->builder,
                                   "/kr/gooroom/IntegrationApplet/modules/tablet/tablet-control.ui",
                                   &error);
	if (error) {
		g_error_free (error);
		return;
	}

	priv->control = GET_WIDGET (priv->builder, "control");
	priv->sw_tablet = GET_WIDGET (priv->builder, "sw_tablet");
	priv->icon_tablet = GET_WIDGET (priv->builder, "icon_tablet");
	priv->init_tablet_mode = is_tablet_mode ();

	gtk_switch_set_active (GTK_SWITCH (priv->sw_tablet), priv->init_tablet_mode);

	gtk_size_group_add_widget (size_group, priv->icon_tablet);

	g_signal_connect (priv->sw_tablet, "state-set",
                      G_CALLBACK (tablet_mode_changed_cb), module);

	gtk_widget_show_all (priv->control);
}

static void
tablet_module_finalize (GObject *object)
{
	TabletModule *module = TABLET_MODULE (object);
	TabletModulePrivate *priv = module->priv;

	g_clear_object (&priv->builder);

	G_OBJECT_CLASS (tablet_module_parent_class)->finalize (object);
}

static void
tablet_module_class_init (TabletModuleClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = tablet_module_finalize;

	signals[LAUNCH_COMMAND] = g_signal_new ("launch-command",
                                            MODULE_TYPE_TABLET,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(TabletModuleClass,
                                            launch_command),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);
}

static void
tablet_module_init (TabletModule *module)
{
	GError *error = NULL;
	TabletModulePrivate *priv;

	module->priv = priv = tablet_module_get_instance_private (module);

	priv->control = NULL;

	priv->builder = gtk_builder_new ();
	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);
}

TabletModule *
tablet_module_new (void)
{
	return g_object_new (MODULE_TYPE_TABLET, NULL);
}

GtkWidget *
tablet_module_control_new (TabletModule *module, GtkSizeGroup *size_group)
{
	g_return_val_if_fail (module != NULL, NULL);

	TabletModulePrivate *priv = module->priv;

	build_control_ui (module, size_group);

	return priv->control;
}

void
tablet_module_control_destroy (TabletModule *module)
{
	g_return_if_fail (module != NULL);

	TabletModulePrivate *priv = module->priv;

	if (priv->control) {
		gtk_widget_destroy (priv->control);
		priv->control = NULL;
	}
}
