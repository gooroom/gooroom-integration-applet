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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <glib/gi18n-lib.h>

#include "common.h"
#include "endsession-module.h"

#define GET_WIDGET(builder, x) GTK_WIDGET (gtk_builder_get_object (builder, x))


struct _EndSessionModulePrivate
{
	GtkBuilder *builder;

	GtkWidget  *control;

	GtkWidget *btn_logout;
	GtkWidget *btn_suspend;
	GtkWidget *btn_hibernate;
	GtkWidget *btn_restart;
	GtkWidget *btn_shutdown;
};

enum {
	LAUNCH_COMMAND,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (EndSessionModule, endsession_module, G_TYPE_OBJECT)



static GDBusProxy *
login1_proxy_get (void)
{
	GDBusProxy *proxy = NULL;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
			G_DBUS_PROXY_FLAGS_NONE,
			NULL, 
			"org.freedesktop.login1",
			"/org/freedesktop/login1",
			"org.freedesktop.login1.Manager",
			NULL,
			NULL);


	return proxy;
}

static gboolean
is_function_available (const char *function)
{
	if (!g_str_equal (function, "CanReboot") &&
	    !g_str_equal (function, "CanPowerOff") &&
	    !g_str_equal (function, "CanSuspend") &&
        !g_str_equal (function, "CanHibernate")) {
		return FALSE;
	}

	gboolean result = FALSE;
	GDBusProxy *proxy = login1_proxy_get ();
	if (proxy) {
		GVariant *r = NULL;
		gchar *string = NULL;

		r = g_dbus_proxy_call_sync (proxy,
				function,
				NULL,
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL,
				NULL);

		if (r) {
			if (g_variant_is_of_type (r, G_VARIANT_TYPE ("(s)"))) {
				g_variant_get (r, "(&s)", &string);
				result = g_str_equal (string, "yes");
			}

			g_variant_unref (r);
		}
	}

	return result;
}

static void
on_endsession_button_clicked_cb (GtkButton *button, gpointer data)
{
	const gchar *opt;
	EndSessionModule *module = ENDSESSION_MODULE (data);
	EndSessionModulePrivate *priv = module->priv;

	if (button == GTK_BUTTON (priv->btn_logout)) {
		opt = "--logout";
	} else if (button == GTK_BUTTON (priv->btn_hibernate)) {
		opt = "--hibernate";
	} else if (button == GTK_BUTTON (priv->btn_suspend)) {
		opt = "--suspend";
	} else if (button == GTK_BUTTON (priv->btn_restart)) {
		opt = "--reboot";
	} else if (button == GTK_BUTTON (priv->btn_shutdown)) {
		opt = "--poweroff";
	} else {
		opt = NULL;
	}

	if (opt) {
		gchar *glc = g_find_program_in_path ("gooroom-logout-command");
		if (glc) {
			gchar *command = g_strdup_printf ("%s %s --delay=500", glc, opt);
			g_signal_emit (G_OBJECT (module), signals[LAUNCH_COMMAND], 0, command);
			g_free (command);
		}
		g_free (glc);
	}
}

static void
build_control_ui (EndSessionModule *module)
{
	GError *error = NULL;
	EndSessionModulePrivate *priv = module->priv;

	if (!priv->control) {
		gtk_builder_add_from_resource (priv->builder,
			"/kr/gooroom/IntegrationApplet/modules/endsession/endsession-control.ui", &error);
		if (error) {
			g_error_free (error);
			return;
		}

		priv->control       = GET_WIDGET (priv->builder, "control");
		priv->btn_logout    = GET_WIDGET (priv->builder, "btn_logout");
		priv->btn_hibernate = GET_WIDGET (priv->builder, "btn_hibernate");
		priv->btn_suspend   = GET_WIDGET (priv->builder, "btn_suspend");
		priv->btn_restart   = GET_WIDGET (priv->builder, "btn_restart");
		priv->btn_shutdown  = GET_WIDGET (priv->builder, "btn_shutdown");

		g_signal_connect (G_OBJECT (priv->btn_logout), "clicked",
				G_CALLBACK (on_endsession_button_clicked_cb), module);

		if (is_function_available ("CanHibernate")) {
			gtk_widget_show (priv->btn_hibernate);
			g_signal_connect (G_OBJECT (priv->btn_hibernate), "clicked",
					G_CALLBACK (on_endsession_button_clicked_cb), module);
		}

		if (is_function_available ("CanSuspend")) {
			gtk_widget_show (priv->btn_suspend);
			g_signal_connect (G_OBJECT (priv->btn_suspend), "clicked",
					G_CALLBACK (on_endsession_button_clicked_cb), module);
		}

		if (is_function_available ("CanReboot")) {
			gtk_widget_show (priv->btn_restart);
			g_signal_connect (G_OBJECT (priv->btn_restart), "clicked",
					G_CALLBACK (on_endsession_button_clicked_cb), module);
		}

		if (is_function_available ("CanPowerOff")) {
			gtk_widget_show (priv->btn_shutdown);
			g_signal_connect (G_OBJECT (priv->btn_shutdown), "clicked",
					G_CALLBACK (on_endsession_button_clicked_cb), module);
		}
	}
}

static void
endsession_module_finalize (GObject *object)
{
	EndSessionModule *module = ENDSESSION_MODULE (object);
	EndSessionModulePrivate *priv = module->priv;

	endsession_module_control_destroy (module);

	g_clear_object (&priv->builder);

	G_OBJECT_CLASS (endsession_module_parent_class)->finalize (object);
}

static void
endsession_module_init (EndSessionModule *module)
{
	EndSessionModulePrivate *priv;
	module->priv = priv = endsession_module_get_instance_private (module);

	priv->control = NULL;

	priv->builder = gtk_builder_new ();
	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);
}

static void
endsession_module_class_init (EndSessionModuleClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = endsession_module_finalize;

	signals[LAUNCH_COMMAND] = g_signal_new ("launch-command",
                                            MODULE_TYPE_ENDSESSION,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(EndSessionModuleClass,
                                            launch_command),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);
}

EndSessionModule *
endsession_module_new (void)
{
	return g_object_new (MODULE_TYPE_ENDSESSION, NULL);
}

GtkWidget *
endsession_module_control_new (EndSessionModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	EndSessionModulePrivate *priv = module->priv;

	build_control_ui (module);

	gtk_widget_show (priv->control);

	return priv->control;
}

void
endsession_module_control_destroy (EndSessionModule *module)
{
	g_return_if_fail (module != NULL);

	EndSessionModulePrivate *priv = module->priv;

	if (priv->control) {
		gtk_widget_destroy (priv->control);
		priv->control = NULL;
	}
}
