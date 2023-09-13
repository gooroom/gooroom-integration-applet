/*
 *  Copyright (C) 2015-2023 Gooroom <gooroom@gooroom.kr>
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <glib/gi18n-lib.h>

#include <act/act.h>

#include "common.h"
#include "user-module.h"

#define CLEAN_MODE_HOME_DIR "/tmp/.cleanmode"

#define GET_WIDGET(builder, x) GTK_WIDGET (gtk_builder_get_object (builder, x))


struct _UserModulePrivate
{
	GtkBuilder *builder;

	GtkWidget  *tray;
	GtkWidget  *user_name;
	GtkWidget  *lbl_clean_mode;
	GtkWidget  *lbl_tablet_mode;
	GtkWidget  *img_status;
	GtkWidget  *control;

	ActUserManager  *um;

	guint update_timeout_id;
};

G_DEFINE_TYPE_WITH_PRIVATE (UserModule, user_module, G_TYPE_OBJECT)



static GdkPixbuf *
get_user_face (const gchar *icon, gint size)
{
	GdkPixbuf *face = NULL;

	if (icon) {
		face = gdk_pixbuf_new_from_file_at_scale (icon, size, size, TRUE, NULL);
	}

	return face;
}

static gboolean
user_info_update (gpointer user_data)
{
	const char *user_name;
	const char *icon_name = NULL;

	UserModule *module = USER_MODULE (user_data);
	UserModulePrivate *priv = module->priv;

	if (!act_user_manager_no_service (priv->um)) {
		ActUser *user = act_user_manager_get_user_by_id (priv->um, getuid ());
		if (user) {
			if (!act_user_is_loaded (user))
				return FALSE;

			icon_name = act_user_get_icon_file (user);
			user_name = act_user_get_real_name (user);
			if (!user_name)
				user_name = act_user_get_user_name (user);
		} else {
			user_name = NULL;
		}
	} else {
		user_name = NULL;
	}

	if (priv->tray) {
		if (g_file_test (CLEAN_MODE_HOME_DIR, G_FILE_TEST_EXISTS)) {
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->tray),
                                          "integrationapplet-users-cleanmode",
                                          GTK_ICON_SIZE_BUTTON);
		} else {
			GdkPixbuf *pix = get_user_face (icon_name, TRAY_ICON_SIZE);
			if (pix) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (priv->tray), pix);
				g_object_unref (G_OBJECT (pix));
			}
		}
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
	}

	if (priv->control) {
		const gchar *s = user_name ? user_name : _("Unknown");

		if (g_file_test (CLEAN_MODE_HOME_DIR, G_FILE_TEST_EXISTS)) {
			gtk_image_set_from_icon_name (GTK_IMAGE (priv->img_status),
                                          "integrationapplet-users-cleanmode",
                                          GTK_ICON_SIZE_BUTTON);
			gtk_image_set_pixel_size (GTK_IMAGE (priv->img_status), STATUS_ICON_SIZE);

			gchar *markup = g_markup_printf_escaped ("<b><span foreground=\"#ffffff\">%s</span></b>", s);
			gtk_label_set_markup (GTK_LABEL (priv->user_name), markup);
			g_free (markup);

			gtk_widget_show (priv->lbl_clean_mode);
		} else {
			GdkPixbuf *pix = get_user_face (icon_name, STATUS_ICON_SIZE);
			if (pix) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (priv->img_status), pix);
				g_object_unref (G_OBJECT (pix));
			}

			gtk_label_set_text (GTK_LABEL (priv->user_name), s);

			gtk_widget_hide (priv->lbl_clean_mode);
		}
	}

	return TRUE;
}

static gboolean
update_user_info_continually (gpointer user_data)
{
	UserModule *module = USER_MODULE (user_data);
	UserModulePrivate *priv = module->priv;

	gboolean loaded = FALSE;
	g_object_get (priv->um, "is-loaded", &loaded, NULL);
	if (loaded) {
		if (user_info_update (module))
			return FALSE;
	}

	return TRUE;
}

static void
user_changed (gpointer user_data, ActUser *user)
{
	user_info_update (user_data);
}

static void
build_control_ui (UserModule *module, GtkSizeGroup *size_group)
{
	GError *error = NULL;
	UserModulePrivate *priv = module->priv;

	gtk_builder_add_from_resource (priv->builder,
                                   "/kr/gooroom/IntegrationApplet/modules/user/user-control.ui",
                                   &error);
	if (error) {
		g_error_free (error);
		return;
	}

	priv->control = GET_WIDGET (priv->builder, "control");
	priv->user_name = GET_WIDGET (priv->builder, "lbl_user_name");
	priv->lbl_clean_mode = GET_WIDGET (priv->builder, "lbl_clean_mode");
	priv->lbl_tablet_mode = GET_WIDGET (priv->builder, "lbl_tablet_mode");
	priv->img_status = GET_WIDGET (priv->builder, "img_status");

	gtk_size_group_add_widget (size_group, priv->img_status);

	if (is_tablet_mode ())
		gtk_widget_show (priv->lbl_tablet_mode);
}

static void
user_module_finalize (GObject *object)
{
	UserModule *module = USER_MODULE (object);
	UserModulePrivate *priv = module->priv;

	user_module_control_destroy (module);

	g_clear_object (&priv->builder);
	g_clear_handle_id (&priv->update_timeout_id, g_source_remove);

	G_OBJECT_CLASS (user_module_parent_class)->finalize (object);
}

static void
user_module_init (UserModule *module)
{
	UserModulePrivate *priv;
	module->priv = priv = user_module_get_instance_private (module);

	priv->tray              = NULL;
	priv->user_name         = NULL;
	priv->lbl_clean_mode    = NULL;
	priv->lbl_tablet_mode   = NULL;
	priv->control           = NULL;
	priv->update_timeout_id = 0;

	priv->builder = gtk_builder_new ();
	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);

	priv->um = act_user_manager_get_default ();

	g_signal_connect_object (priv->um, "user-changed",
                             G_CALLBACK (user_changed), module, G_CONNECT_SWAPPED);
}

static void
user_module_class_init (UserModuleClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = user_module_finalize;
}

UserModule *
user_module_new (void)
{
	return g_object_new (MODULE_TYPE_USER, NULL);
}

GtkWidget *
user_module_tray_new (UserModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	UserModulePrivate *priv = module->priv;

	if (!priv->tray) {
		if (g_file_test (CLEAN_MODE_HOME_DIR, G_FILE_TEST_EXISTS)) {
			priv->tray = gtk_image_new_from_icon_name ("integrationapplet-users-cleanmode",
                                                       GTK_ICON_SIZE_BUTTON);
		} else {
			priv->tray = gtk_image_new_from_icon_name ("integrationapplet-users",
                                                       GTK_ICON_SIZE_BUTTON);
		}
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
	}

	gtk_widget_show (priv->tray);

	if (update_user_info_continually (module)) {
		g_clear_handle_id (&priv->update_timeout_id, g_source_remove);
		priv->update_timeout_id = g_timeout_add (500, (GSourceFunc)update_user_info_continually, module);
	}

	return priv->tray;
}

GtkWidget *
user_module_control_new (UserModule *module, GtkSizeGroup *size_group)
{
	g_return_val_if_fail (module != NULL, NULL);

	UserModulePrivate *priv = module->priv;

	build_control_ui (module, size_group);

	gtk_widget_show_all (priv->control);

	if (update_user_info_continually (module)) {
		g_clear_handle_id (&priv->update_timeout_id, g_source_remove);
		priv->update_timeout_id = g_timeout_add (500, (GSourceFunc)update_user_info_continually, module);
	}

	return priv->control;
}

void
user_module_control_destroy (UserModule *module)
{
	g_return_if_fail (module != NULL);

	UserModulePrivate *priv = module->priv;

	if (priv->control) {
		if (priv->user_name) {
			gtk_widget_destroy (priv->user_name);
			priv->user_name = NULL;
		}

		gtk_widget_destroy (priv->control);
		priv->control = NULL;
	}
}
