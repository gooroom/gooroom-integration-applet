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

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "common.h"
#include "gooroom-calendar.h"
#include "datetime-module.h"

#define GET_WIDGET(builder, x) GTK_WIDGET (gtk_builder_get_object (builder, x))

struct _DateTimeModulePrivate
{
	GtkWidget  *tray;
	GtkWidget  *control;
	GtkWidget  *control_menu;
	GtkWidget  *details_label;

	GtkBuilder *builder;

	GSettings  *clock_settings;

	guint       clock_timer;

	gboolean    use_ampm;
};

enum {
	LAUNCH_DESKTOP,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE_WITH_PRIVATE (DateTimeModule, datetime_module, G_TYPE_OBJECT)


/*
 * Translate @str according to the locale defined by LC_TIME; unlike
 * dcgettext(), the translation is still taken from the LC_MESSAGES
 * catalogue and not the LC_TIME one.
 */
const gchar *
translate_time_format_string (const char *str)
{
  const char *locale = g_getenv ("LC_TIME");
  const char *res;
  char *sep;
  locale_t old_loc;
  locale_t loc = (locale_t)0;

  if (locale)
    loc = newlocale (LC_MESSAGES_MASK, locale, (locale_t)0);

  old_loc = uselocale (loc);

  sep = strchr (str, '\004');
  res = g_dpgettext (GETTEXT_PACKAGE, str, sep ? sep - str + 1 : 0);

  uselocale (old_loc);

  if (loc != (locale_t)0)
    freelocale (loc);

  return res;
}

gboolean
get_use_ampm (gpointer data)
{
	DateTimeModule *module = DATETIME_MODULE (data);
	DateTimeModulePrivate *priv = module->priv;

	return priv->use_ampm;
}

gboolean
clock_timeout_thread (gpointer data)
{
	gchar *fm, *markup;
	GDateTime *dt = NULL;
	DateTimeModule *module = DATETIME_MODULE (data);
	DateTimeModulePrivate *priv = module->priv;

	dt = g_date_time_new_now_local ();
	if (dt) {
		if (priv->tray) {
			if (priv->use_ampm) //12-hour mode
				fm = g_date_time_format (dt, translate_time_format_string (N_("%l:%M %p")));
			else
				fm = g_date_time_format (dt, "%R");
			markup = g_markup_printf_escaped ("<b>%s</b>", fm);
			gtk_label_set_markup (GTK_LABEL (priv->tray), markup);

			g_free (fm);
			g_free (markup);
		}

		if (priv->details_label) {
			if (priv->use_ampm) // 12-hour mode
				fm = g_date_time_format (dt, translate_time_format_string (N_("%B %-d %Y   %l:%M:%S %p")));
			else
				fm = g_date_time_format (dt, translate_time_format_string (N_("%B %-d %Y   %T")));
			markup = g_markup_printf_escaped ("<b>%s</b>", fm);
			gtk_label_set_markup (GTK_LABEL (priv->details_label), markup);

			g_free (fm);
			g_free (markup);
		}

		g_date_time_unref (dt);
	}

	return TRUE;
}

static void
on_settings_clicked_cb (GtkButton *button, gpointer data)
{
	GSettingsSchema *schema = NULL;
	DateTimeModule *module = DATETIME_MODULE (data);

	schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (), "org.gnome.ControlCenter", TRUE);

	if (schema) {
		GSettings *settings = g_settings_new_full (schema, NULL, NULL);
		g_settings_set_string (settings, "last-panel", "datetime");
		g_object_unref (settings);
		g_settings_schema_unref (schema);
	}

	g_signal_emit (G_OBJECT (module), signals[LAUNCH_DESKTOP], 0, "gnome-datetime-panel.desktop");
}

void
build_control_ui (DateTimeModule *module, GtkSizeGroup *size_group)
{
    GtkWidget *box, *icon, *label;
	DateTimeModulePrivate *priv = module->priv;
	GtkStyleContext *context;

	priv->control = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->control), GTK_RELIEF_NONE);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box), 0);
	gtk_container_add (GTK_CONTAINER (priv->control), box);

	icon = gtk_image_new_from_icon_name ("preferences-system-time-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_image_set_pixel_size (GTK_IMAGE (icon), STATUS_ICON_SIZE);

    context = gtk_widget_get_style_context (GTK_WIDGET (icon));
	gtk_style_context_add_class (context, "general-box-icon");

	if (size_group)
		gtk_size_group_add_widget (size_group, icon);
	gtk_box_pack_start (GTK_BOX (box), icon, FALSE, FALSE, 0);

	priv->details_label = label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (priv->details_label), 0);
	gtk_label_set_max_width_chars (GTK_LABEL (priv->details_label), 1);
	gtk_label_set_ellipsize (GTK_LABEL (priv->details_label), PANGO_ELLIPSIZE_END);
	gtk_label_set_line_wrap (GTK_LABEL (priv->details_label), FALSE);

	gtk_box_pack_start (GTK_BOX (box), priv->details_label, TRUE, TRUE, 0);

	icon = gtk_image_new_from_icon_name ("go-next-page-symbolic", GTK_ICON_SIZE_BUTTON);

    context = gtk_widget_get_style_context (GTK_WIDGET (icon));
	gtk_style_context_add_class (context, "go-next-page");

	gtk_image_set_pixel_size (GTK_IMAGE (icon), STATUS_ICON_SIZE);
	gtk_box_pack_end (GTK_BOX (box), icon, FALSE, FALSE, 0);
}

static void
build_control_menu_ui (DateTimeModule *module)
{
	GError *error = NULL;
	GtkWidget *calendar, *inner_box, *btn_settings;
	DateTimeModulePrivate *priv = module->priv;

	gtk_builder_add_from_resource (priv->builder, "/kr/gooroom/IntegrationApplet/modules/datetime/datetime-control-menu.ui", &error);
	if (error) {
		g_error_free (error);
		return;
	}

	priv->control_menu = GET_WIDGET (priv->builder, "control_menu");
	inner_box          = GET_WIDGET (priv->builder, "inner_box");
	btn_settings       = GET_WIDGET (priv->builder, "btn_settings");

	calendar = gooroom_calendar_new ();
	gtk_widget_show (calendar);

	gtk_widget_set_name (inner_box, "inner_box");
	gtk_box_pack_start (GTK_BOX (inner_box), calendar, FALSE, FALSE, 0);
	gtk_box_reorder_child (GTK_BOX (inner_box), calendar, 0);

	if (priv->clock_timer) {
		g_source_remove (priv->clock_timer);
		priv->clock_timer = 0;
	}

	clock_timeout_thread (module);

	priv->clock_timer = gdk_threads_add_timeout (1000, (GSourceFunc) clock_timeout_thread, module);

	g_signal_connect (G_OBJECT (btn_settings), "clicked", G_CALLBACK (on_settings_clicked_cb), module);
}

static void
clock_settings_changed_cb (GSettings *settings, gchar *key, gpointer data)
{
	DateTimeModule *module = DATETIME_MODULE (data);
	DateTimeModulePrivate *priv = module->priv;

	gint clock_format;

	clock_format = g_settings_get_enum (priv->clock_settings, "clock-format");

	priv->use_ampm = (clock_format == 1);
}

static void
datetime_module_finalize (GObject *object)
{
	DateTimeModule *module = DATETIME_MODULE (object);
	DateTimeModulePrivate *priv = module->priv;

	if (priv->clock_timer) {
		g_source_remove (priv->clock_timer);
		priv->clock_timer = 0;
	}

	g_clear_object (&priv->builder);
	g_clear_object (&priv->clock_settings);

	datetime_module_control_destroy (module);

	G_OBJECT_CLASS (datetime_module_parent_class)->finalize (object);
}

static void
datetime_module_class_init (DateTimeModuleClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = datetime_module_finalize;

	signals[LAUNCH_DESKTOP] = g_signal_new ("launch-desktop",
                                            MODULE_TYPE_DATETIME,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(DateTimeModuleClass,
                                            launch_desktop),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);
}

static void
datetime_module_init (DateTimeModule *module)
{
	DateTimeModulePrivate *priv;
	module->priv = priv = datetime_module_get_instance_private (module);

	priv->tray          = NULL;
	priv->control       = NULL;
	priv->control_menu  = NULL;
	priv->details_label = NULL;
	priv->clock_timer   = 0;
	priv->use_ampm      = FALSE;

	priv->builder = gtk_builder_new ();
	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);

	/* Clock settings */
	priv->clock_settings = g_settings_new ("org.gnome.desktop.interface");

	clock_settings_changed_cb (priv->clock_settings, "clock-format", module);

	g_signal_connect (priv->clock_settings, "changed::clock-format",
                      G_CALLBACK (clock_settings_changed_cb), module);
}

DateTimeModule *
datetime_module_new (void)
{
	return g_object_new (MODULE_TYPE_DATETIME, NULL);
}

GtkWidget *
datetime_module_tray_new (DateTimeModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	DateTimeModulePrivate *priv = module->priv;

	if (priv->clock_timer != 0) {
		g_source_remove (priv->clock_timer);
		priv->clock_timer = 0;
	}

	if (!priv->tray) {
		priv->tray = gtk_label_new (NULL);
	}

	gtk_widget_show (priv->tray);

	clock_timeout_thread (module);

	priv->clock_timer = gdk_threads_add_timeout (1000, (GSourceFunc) clock_timeout_thread, module);

	return priv->tray;
}

GtkWidget *
datetime_module_control_new (DateTimeModule *module, GtkSizeGroup *size_group)
{
	g_return_val_if_fail (module != NULL, NULL);

	DateTimeModulePrivate *priv = module->priv;

	if (priv->clock_timer) {
		g_source_remove (priv->clock_timer);
		priv->clock_timer = 0;
	}

	build_control_ui (module, size_group);

	gtk_widget_show_all (priv->control);

	clock_timeout_thread (module);

	priv->clock_timer = gdk_threads_add_timeout (1000, (GSourceFunc) clock_timeout_thread, module);

	return priv->control;
}

GtkWidget *
datetime_module_control_menu_new (DateTimeModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	DateTimeModulePrivate *priv = module->priv;

	build_control_menu_ui (module);

	gtk_widget_show (priv->control_menu);

	return priv->control_menu;
}

void
datetime_module_control_destroy (DateTimeModule *module)
{
	g_return_if_fail (module != NULL);

	DateTimeModulePrivate *priv = module->priv;

	if (priv->control) {
		if (priv->details_label) {
			gtk_widget_destroy (priv->details_label);
			priv->details_label = NULL;
		}
		gtk_widget_destroy (priv->control);
		priv->control = NULL;
	}
}

void
datetime_module_control_menu_destroy (DateTimeModule *module)
{
	g_return_if_fail (module != NULL);

	DateTimeModulePrivate *priv = module->priv;

	if (priv->control_menu) {
		gtk_widget_destroy (priv->control_menu);
		priv->control_menu = NULL;
	}
}
