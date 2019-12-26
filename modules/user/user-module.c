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

#include <act/act.h>

#include "common.h"
#include "user-module.h"

#define GET_WIDGET(builder, x) GTK_WIDGET (gtk_builder_get_object (builder, x))


struct _UserModulePrivate
{
	GtkBuilder *builder;

	GtkWidget  *tray;
	GtkWidget  *user_name;
	GtkWidget  *img_status;
	GtkWidget  *control;

	ActUserManager  *um;
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

static void
user_info_update (ActUserManager *um, GParamSpec *pspec, gpointer data)
{
	g_return_if_fail (data != NULL);

	const char *user_name, *icon_name;

	UserModule *module = USER_MODULE (data);
	UserModulePrivate *priv = module->priv;

	if (!act_user_manager_no_service (um)) {
		ActUser *user = act_user_manager_get_user_by_id (um, getuid ());
		if (user) {
			icon_name = act_user_get_icon_file (user);
			user_name = act_user_get_real_name (user);
			if (user_name == NULL)
				user_name = act_user_get_user_name (user);
		} else {
			user_name = NULL;
		}
	} else {
		user_name = NULL;
	}

	if (priv->tray) {
		GdkPixbuf *pix = get_user_face (icon_name, TRAY_ICON_SIZE);
		if (pix) {
			gtk_image_set_from_pixbuf (GTK_IMAGE (priv->tray), pix);
			g_object_unref (G_OBJECT (pix));
		}
	}

	if (priv->control) {
		if (priv->user_name) {
			const gchar *s = user_name ? user_name : _("Unknown");
			gchar *markup = g_strdup_printf ("<b>%s</b>", s);
			gtk_label_set_markup (GTK_LABEL (priv->user_name), markup);
			g_free (markup);
		}
		if (priv->img_status) {
			GdkPixbuf *pix = get_user_face (icon_name, 24);
			if (pix) {
				gtk_image_set_from_pixbuf (GTK_IMAGE (priv->img_status), pix);
				g_object_unref (G_OBJECT (pix));
			}
		}
	}
}

static void
build_control_ui (UserModule *module, GtkSizeGroup *size_group)
{
	GError *error = NULL;
	GtkWidget *btn_face;

	UserModulePrivate *priv = module->priv;

	gtk_builder_add_from_resource (priv->builder, "/kr/gooroom/IntegrationApplet/modules/user/user-control.ui", &error);
	if (error) {
		g_error_free (error);
		return;
	}

	priv->control = GET_WIDGET (priv->builder, "control");
	btn_face = GET_WIDGET (priv->builder, "btn_face");
	priv->user_name = GET_WIDGET (priv->builder, "lbl_user_name");
	priv->img_status = GET_WIDGET (priv->builder, "img_status");

	if (size_group)
		gtk_size_group_add_widget (size_group, btn_face);
}

static void
user_module_finalize (GObject *object)
{
	UserModule *module = USER_MODULE (object);
	UserModulePrivate *priv = module->priv;

	user_module_control_destroy (module);

	g_clear_object (&priv->builder);

	G_OBJECT_CLASS (user_module_parent_class)->finalize (object);
}

static void
user_module_init (UserModule *module)
{
	UserModulePrivate *priv;
	module->priv = priv = user_module_get_instance_private (module);

	priv->tray         = NULL;
	priv->user_name    = NULL;
	priv->control      = NULL;

	priv->builder = gtk_builder_new ();
	gtk_builder_set_translation_domain (priv->builder, GETTEXT_PACKAGE);

	priv->um = act_user_manager_get_default ();

	g_signal_connect (priv->um, "notify::is-loaded", G_CALLBACK (user_info_update), module);
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
		priv->tray = gtk_image_new_from_icon_name ("avatar-default", GTK_ICON_SIZE_BUTTON);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
	}

	gboolean loaded = FALSE;
	g_object_get (priv->um, "is-loaded", &loaded, NULL);
	if (loaded)
		user_info_update (priv->um, NULL, module);

	gtk_widget_show (priv->tray);

	return priv->tray;
}

GtkWidget *
user_module_control_new (UserModule *module, GtkSizeGroup *size_group)
{
	g_return_val_if_fail (module != NULL, NULL);

	UserModulePrivate *priv = module->priv;

	build_control_ui (module, size_group);

	gboolean loaded = FALSE;
	g_object_get (priv->um, "is-loaded", &loaded, NULL);
	if (loaded)
		user_info_update (priv->um, NULL, module);

	gtk_widget_show_all (priv->control);

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
