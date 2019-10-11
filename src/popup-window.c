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
#include <config.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif


#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "common.h"
#include "popup-window.h"

enum {
	CONTROL_TYPE_USER = 0,
	CONTROL_TYPE_VOLUME,
	CONTROL_TYPE_SECURITY,
	CONTROL_TYPE_BRIGHTNESS,
	CONTROL_TYPE_BATTERY,
	CONTROL_TYPE_DATETIME,
	CONTROL_TYPE_NIMF
};



struct _PopupWindowPrivate
{
	GtkWidget *box_user;
	GtkWidget *box_control;
	GtkWidget *box_general;

	GtkWidget *btn_endsession_back;
	GtkWidget *btn_security_back;
	GtkWidget *btn_nimf_back;
	GtkWidget *btn_datetime_back;
	GtkWidget *btn_settings;
	GtkWidget *btn_screenlock;
	GtkWidget *btn_endsession;

	GtkWidget *stack;

	UserModule       *user_module;
	SoundModule      *sound_module;
	SecurityModule   *security_module;
	PowerModule      *power_module;
	DateTimeModule   *datetime_module;
	EndSessionModule *endsession_module;
	NimfModule       *nimf_module;

	gboolean block_focus_out_event;

	GtkSizeGroup *size_group;

	int x;
	int y;
	int width;
	int height;
};

enum {
    CLOSED,
    LAUNCH_DESKTOP,
    LAUNCH_COMMAND,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define CONTROL_WIDGET_NAME "module-widget"


G_DEFINE_TYPE_WITH_PRIVATE (PopupWindow, popup_window, GTK_TYPE_WINDOW)


static void
grab_pointer (GtkWidget *widget)
{
	GdkDisplay *display = gdk_display_get_default ();

	gdk_seat_grab (gdk_display_get_default_seat (display),
                   gtk_widget_get_window (widget),
                   GDK_SEAT_CAPABILITY_ALL_POINTING,
                   TRUE, NULL, NULL, NULL, NULL);
}

static void
ungrab_pointer ()
{
	GdkDisplay *display = gdk_display_get_default();
	gdk_seat_ungrab (gdk_display_get_default_seat (display));
}

static void
destroy_control_widget (PopupWindow *window)
{
	PopupWindowPrivate *priv = window->priv;

	if (priv->user_module) {
		user_module_control_destroy (priv->user_module);
	}

	if (priv->sound_module) {
		sound_module_control_destroy (priv->sound_module);
	}

	if (priv->security_module) {
		security_module_control_destroy (priv->security_module);
		security_module_control_menu_destroy (priv->security_module);
	}

	if (priv->power_module) {
		power_module_brightness_control_destroy (priv->power_module);
		power_module_battery_control_destroy (priv->power_module);
	}

	if (priv->datetime_module) {
		datetime_module_control_destroy (priv->datetime_module);
	}

	if (priv->endsession_module) {
		endsession_module_control_destroy (priv->endsession_module);
	}

	if (priv->nimf_module) {
		nimf_module_control_destroy (priv->nimf_module);
		nimf_module_control_menu_destroy (priv->nimf_module);
	}
}

static void
add_control_widget (PopupWindow *window,
                    GtkWidget   *control,
                    gint         type)
{
	PopupWindowPrivate *priv = window->priv;

	if (!control) return;

	switch (type)
	{
		case CONTROL_TYPE_USER:
		{
			gtk_box_pack_start (GTK_BOX (priv->box_user), control, TRUE, FALSE, 0);
			gtk_widget_show (control);
			break;
		}

		case CONTROL_TYPE_VOLUME:
		case CONTROL_TYPE_BRIGHTNESS:
		{
			gtk_box_pack_start (GTK_BOX (priv->box_control), control, TRUE, FALSE, 0);
			break;
		}

		case CONTROL_TYPE_SECURITY:
		case CONTROL_TYPE_BATTERY:
		case CONTROL_TYPE_DATETIME:
		case CONTROL_TYPE_NIMF:
		{
			gtk_box_pack_start (GTK_BOX (priv->box_general), control, TRUE, FALSE, 0);
			break;
		}

		default:
		break;
	}
}

static void
on_endsession_back_button_clicked_cb (GtkWidget *button, gpointer data)
{
	PopupWindow *window = POPUP_WINDOW (data);
	PopupWindowPrivate *priv = window->priv;

	gtk_stack_set_visible_child_name (GTK_STACK (priv->stack), "main-page");
	if (priv->endsession_module) {
		endsession_module_control_destroy (priv->endsession_module);
	}
}

static void
on_security_back_button_clicked_cb (GtkButton *button, gpointer data)
{
	PopupWindow *window = POPUP_WINDOW (data);
	PopupWindowPrivate *priv = window->priv;

	gtk_stack_set_visible_child_name (GTK_STACK (priv->stack), "main-page");
	if (priv->security_module) {
		security_module_control_menu_destroy (priv->security_module);
	}
}

static void
on_datetime_back_button_clicked_cb (GtkButton *button, gpointer data)
{
	PopupWindow *window = POPUP_WINDOW (data);
	PopupWindowPrivate *priv = window->priv;

	gtk_stack_set_visible_child_name (GTK_STACK (priv->stack), "main-page");
	if (priv->datetime_module) {
		datetime_module_control_menu_destroy (priv->datetime_module);
	}
}

static void
on_nimf_back_button_clicked_cb (GtkButton *button, gpointer data)
{
	PopupWindow *window = POPUP_WINDOW (data);
	PopupWindowPrivate *priv = window->priv;

	gtk_stack_set_visible_child_name (GTK_STACK (priv->stack), "main-page");
	if (priv->nimf_module) {
		nimf_module_control_menu_destroy (priv->nimf_module);
	}
}

static void
on_security_button_clicked_cb (GtkButton *button, gpointer data)
{
	PopupWindow *window = POPUP_WINDOW (data);
	PopupWindowPrivate *priv = window->priv;

	if (priv->security_module) {
		GtkWidget *w = NULL;
		w = security_module_control_menu_new (priv->security_module);
		if (w) {
			GtkWidget *child = gtk_stack_get_child_by_name (GTK_STACK (priv->stack), "security-page");
			if (child) {
				gtk_box_pack_end (GTK_BOX (child), w, TRUE, TRUE, 0);
				gtk_stack_set_visible_child (GTK_STACK (priv->stack), child);
			}
		}
	}
}

static void
on_endsession_button_clicked_cb (GtkButton *button, gpointer data)
{
	PopupWindow *window = POPUP_WINDOW (data);
	PopupWindowPrivate *priv = window->priv;

	if (priv->endsession_module) {
		GtkWidget *w = NULL;
		w = endsession_module_control_new (priv->endsession_module);
		if (w) {
			GtkWidget *child = gtk_stack_get_child_by_name (GTK_STACK (priv->stack), "endsession-page");
			if (child) {
				gtk_box_pack_end (GTK_BOX (child), w, TRUE, TRUE, 0);
				gtk_stack_set_visible_child (GTK_STACK (priv->stack), child);
			}
		}
	}
}

static void
on_datetime_button_clicked_cb (GtkButton *button, gpointer data)
{
	PopupWindow *window = POPUP_WINDOW (data);
	PopupWindowPrivate *priv = window->priv;

	if (priv->datetime_module) {
		GtkWidget *w = NULL;
		w = datetime_module_control_menu_new (priv->datetime_module);
		if (w) {
			GtkWidget *child = gtk_stack_get_child_by_name (GTK_STACK (priv->stack), "datetime-page");
			if (child) {
				gtk_box_pack_start (GTK_BOX (child), w, TRUE, TRUE, 0);
				gtk_stack_set_visible_child (GTK_STACK (priv->stack), child);
			}
		}
	}
}

static void
on_nimf_button_clicked_cb (GtkButton *button, gpointer data)
{
	PopupWindow *window = POPUP_WINDOW (data);
	PopupWindowPrivate *priv = window->priv;

	if (priv->nimf_module) {
		GtkWidget *w = NULL;
		w = nimf_module_control_menu_new (priv->nimf_module);
		if (w) {
			GtkWidget *child = gtk_stack_get_child_by_name (GTK_STACK (priv->stack), "nimf-page");
			if (child) {
				gtk_box_pack_start (GTK_BOX (child), w, TRUE, TRUE, 0);
				gtk_stack_set_visible_child (GTK_STACK (priv->stack), child);
			}
		}
	}
}

static void
on_system_button_clicked_cb (GtkButton *button, gpointer data)
{
	PopupWindow *window = POPUP_WINDOW (data);
	PopupWindowPrivate *priv = window->priv;

	if (button == GTK_BUTTON (priv->btn_settings)) {
		GSettingsSchema *schema = NULL;

		schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (), "org.gnome.ControlCenter", TRUE);

		if (schema) {
			GSettings *settings = g_settings_new_full (schema, NULL, NULL);
			g_settings_set_string (settings, "last-panel", "");
			g_object_unref (settings);
			g_settings_schema_unref (schema);
		}
		g_signal_emit (G_OBJECT (window), signals[LAUNCH_DESKTOP], 0, "gnome-control-center.desktop");
	} else if (button == GTK_BUTTON (priv->btn_screenlock)) {
		g_signal_emit (G_OBJECT (window), signals[LAUNCH_COMMAND], 0, "gnome-screensaver-command -l");
	}
}

static gboolean
popup_window_configure_event (GtkWidget         *widget,
                              GdkEventConfigure *event)
{
	PopupWindow *window = POPUP_WINDOW (widget);
	PopupWindowPrivate *priv = window->priv;

	if (event->width && event->height) {
		priv->x = event->x;
		priv->y = event->y;
		priv->width = event->width;
		priv->height = event->height;
	}

	return GTK_WIDGET_CLASS (popup_window_parent_class)->configure_event (widget, event);
}

static gboolean
popup_window_key_press_event (GtkWidget   *widget,
                              GdkEventKey *event)
{
	PopupWindow *window = POPUP_WINDOW (widget);

	if (event->keyval == GDK_KEY_Escape) {
		ungrab_pointer ();
		g_signal_emit (G_OBJECT (window), signals[CLOSED], 0, POPUP_WINDOW_CLOSED);
		return TRUE;
	}

    return FALSE;
}

static gint
popup_window_button_release_event (GtkWidget      *widget,
                                   GdkEventButton *event)
{
	PopupWindow *window = POPUP_WINDOW (widget);
	PopupWindowPrivate *priv = window->priv;

	GtkWidget *toplevel = gtk_widget_get_toplevel (widget);

	if (GTK_IS_WINDOW (toplevel)) {
		GtkWidget *focus = gtk_window_get_focus (GTK_WINDOW (toplevel));

		if (focus && gtk_widget_is_ancestor (focus, widget))
			return gtk_widget_event (focus, (GdkEvent*) event);
	}

	return GTK_WIDGET_CLASS (popup_window_parent_class)->button_release_event (widget, event);
}

static gint
popup_window_button_press_event (GtkWidget      *widget,
                                 GdkEventButton *event)
{
	PopupWindow *window = POPUP_WINDOW (widget);
	PopupWindowPrivate *priv = window->priv;

    // destroy window if user clicks outside
    if ((event->x_root <= priv->x) ||
        (event->y_root <= priv->y) ||
        (event->x_root >= priv->x + priv->width) ||
        (event->y_root >= priv->y + priv->height))
    {
		ungrab_pointer ();
		g_signal_emit (G_OBJECT (window), signals[CLOSED], 0, POPUP_WINDOW_CLOSED);
    }

	return GTK_WIDGET_CLASS (popup_window_parent_class)->button_press_event (widget, event);
}

static gboolean
popup_window_map_event (GtkWidget   *widget,
                        GdkEventAny *event)
{
	PopupWindow *window = POPUP_WINDOW (widget);

	gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);

	grab_pointer (widget);

	return GTK_WIDGET_CLASS (popup_window_parent_class)->map_event (widget, event);
}

static gboolean
popup_window_focus_out_event (GtkWidget     *widget,
                              GdkEventFocus *event)
{
	PopupWindow *window = POPUP_WINDOW (widget);
	PopupWindowPrivate *priv = window->priv;

	if (!priv->block_focus_out_event) {
		ungrab_pointer ();
		g_signal_emit (G_OBJECT (window), signals[CLOSED], 0, POPUP_WINDOW_CLOSED);
	}

	return GTK_WIDGET_CLASS (popup_window_parent_class)->focus_out_event (widget, event);
}

static void
popup_window_init (PopupWindow *window)
{
	PopupWindowPrivate *priv;

	priv = window->priv = popup_window_get_instance_private (window);

	gtk_widget_init_template (GTK_WIDGET (window));

	priv->user_module       = NULL;
	priv->sound_module      = NULL;
	priv->security_module   = NULL;
	priv->power_module      = NULL;
	priv->datetime_module   = NULL;
	priv->endsession_module = NULL;
	priv->nimf_module       = NULL;

	priv->block_focus_out_event = FALSE;

	priv->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	gtk_window_stick (GTK_WINDOW (window));
	gtk_window_set_accept_focus (GTK_WINDOW (window), TRUE);
	gtk_window_set_modal (GTK_WINDOW (window), TRUE);
	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);

	GdkScreen *screen = gtk_window_get_screen (GTK_WINDOW (window));
	if(gdk_screen_is_composited(screen)) {
		GdkVisual *visual = gdk_screen_get_rgba_visual (screen);
		if (visual == NULL)
			visual = gdk_screen_get_system_visual (screen);

		gtk_widget_set_visual (GTK_WIDGET (window), visual);
	}

	g_signal_connect (G_OBJECT (priv->btn_settings), "clicked",
			G_CALLBACK (on_system_button_clicked_cb), window);
	g_signal_connect (G_OBJECT (priv->btn_screenlock), "clicked",
			G_CALLBACK (on_system_button_clicked_cb), window);
	g_signal_connect (G_OBJECT (priv->btn_endsession), "clicked",
			G_CALLBACK (on_endsession_button_clicked_cb), window);
	g_signal_connect (G_OBJECT (priv->btn_endsession_back), "clicked",
			G_CALLBACK (on_endsession_back_button_clicked_cb), window);
	g_signal_connect (G_OBJECT (priv->btn_security_back), "clicked",
			G_CALLBACK (on_security_back_button_clicked_cb), window);
	g_signal_connect (G_OBJECT (priv->btn_nimf_back), "clicked",
			G_CALLBACK (on_nimf_back_button_clicked_cb), window);
	g_signal_connect (G_OBJECT (priv->btn_datetime_back), "clicked",
			G_CALLBACK (on_datetime_back_button_clicked_cb), window);
}

static void
popup_window_dispose (GObject *object)
{
	PopupWindow *window = POPUP_WINDOW (object);

	ungrab_pointer ();

	destroy_control_widget (window);

	G_OBJECT_CLASS (popup_window_parent_class)->dispose (object);
}

static void
popup_window_finalize (GObject *object)
{
	PopupWindow *window = POPUP_WINDOW (object);
	PopupWindowPrivate *priv = window->priv;

	priv->block_focus_out_event = FALSE;

	G_OBJECT_CLASS (popup_window_parent_class)->finalize (object);
}

static void
popup_window_class_init (PopupWindowClass *klass)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	object_class->dispose  = popup_window_dispose;
	object_class->finalize = popup_window_finalize;

	widget_class->focus_out_event = popup_window_focus_out_event;
	widget_class->map_event = popup_window_map_event;
	widget_class->button_press_event = popup_window_button_press_event;
	widget_class->button_release_event = popup_window_button_release_event;
	widget_class->configure_event = popup_window_configure_event;
	widget_class->key_press_event = popup_window_key_press_event;


	signals[CLOSED] = g_signal_new ("closed",
                                    WINDOW_TYPE_POPUP,
                                    G_SIGNAL_RUN_LAST,
                                    G_STRUCT_OFFSET(PopupWindowClass,
                                    closed),
                                    NULL, NULL,
                                    g_cclosure_marshal_VOID__INT,
                                    G_TYPE_NONE, 1,
                                    G_TYPE_INT);

	signals[LAUNCH_DESKTOP] = g_signal_new ("launch-desktop",
                                            WINDOW_TYPE_POPUP,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(PopupWindowClass,
                                            launch_desktop),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);

	signals[LAUNCH_COMMAND] = g_signal_new ("launch-command",
                                            WINDOW_TYPE_POPUP,
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET(PopupWindowClass,
                                            launch_command),
                                            NULL, NULL,
                                            g_cclosure_marshal_VOID__STRING,
                                            G_TYPE_NONE, 1,
                                            G_TYPE_STRING);

	gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
			"/kr/gooroom/IntegrationApplet/ui/popup-window.ui");

	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, box_user);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, box_control);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, box_general);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, btn_settings);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, btn_screenlock);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, btn_endsession);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, btn_endsession_back);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, btn_security_back);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, btn_datetime_back);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, btn_nimf_back);
	gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (klass), PopupWindow, stack);
}

PopupWindow *
popup_window_new (void)
{
	return g_object_new (WINDOW_TYPE_POPUP, NULL);
}

void
popup_window_setup_user (PopupWindow *window,
                         UserModule  *module)
{
	PopupWindowPrivate *priv = window->priv;

	if (MODULE_IS_USER (module)) {
		if (!priv->user_module) {
			priv->user_module = module;
			GtkWidget *w = user_module_control_new (priv->user_module, priv->size_group);
			gtk_widget_set_name (w, CONTROL_WIDGET_NAME);
			add_control_widget (window, w, CONTROL_TYPE_USER);
		}
	}
}

void
popup_window_setup_sound (PopupWindow *window,
                          SoundModule *module)
{
	PopupWindowPrivate *priv = window->priv;

	if (MODULE_IS_SOUND (module)) {
		if (!priv->sound_module) {
			priv->sound_module = module;
			GtkWidget *w = sound_module_control_new (priv->sound_module, priv->size_group);
			gtk_widget_set_name (w, CONTROL_WIDGET_NAME);
			add_control_widget (window, w, CONTROL_TYPE_VOLUME);
		}
	}
}

void
popup_window_setup_security (PopupWindow    *window,
                             SecurityModule *module)
{
	PopupWindowPrivate *priv = window->priv;

	if (MODULE_IS_SECURITY (module)) {
		if (!priv->security_module) {
			priv->security_module = module;
			GtkWidget *w = security_module_control_new (priv->security_module, priv->size_group);
			gtk_widget_set_name (w, CONTROL_WIDGET_NAME);
			add_control_widget (window, w, CONTROL_TYPE_SECURITY);

			g_signal_connect (G_OBJECT (w), "clicked",
					G_CALLBACK (on_security_button_clicked_cb), window);
		}
	}
}

void
popup_window_setup_power (PopupWindow *window,
                          PowerModule *module)
{
	PopupWindowPrivate *priv = window->priv;

	if (MODULE_IS_POWER (module)) {
		if (!priv->power_module) {
			priv->power_module = module;
			GtkWidget *w = NULL;
			w = power_module_brightness_control_new (priv->power_module, priv->size_group);
			gtk_widget_set_name (w, CONTROL_WIDGET_NAME);
			add_control_widget (window, w, CONTROL_TYPE_BRIGHTNESS);

			w = power_module_battery_control_new (priv->power_module, priv->size_group);
			gtk_widget_set_name (w, CONTROL_WIDGET_NAME);
			add_control_widget (window, w, CONTROL_TYPE_BATTERY);
		}
	}
}

void
popup_window_setup_datetime (PopupWindow    *window,
                             DateTimeModule *module)
{
	PopupWindowPrivate *priv = window->priv;

	if (MODULE_IS_DATETIME (module)) {
		if (!priv->datetime_module) {
			priv->datetime_module = module;
			GtkWidget *w = datetime_module_control_new (priv->datetime_module, priv->size_group);
			gtk_widget_set_name (w, CONTROL_WIDGET_NAME);
			add_control_widget (window, w, CONTROL_TYPE_DATETIME);

			g_signal_connect (G_OBJECT (w), "clicked",
					G_CALLBACK (on_datetime_button_clicked_cb), window);
		}
	}
}

void
popup_window_setup_endsession (PopupWindow      *window,
                               EndSessionModule *module)
{
	PopupWindowPrivate *priv = window->priv;

	if (MODULE_IS_ENDSESSION (module)) {
		if (!priv->endsession_module) {
			priv->endsession_module = module;
		}
	}
}

void
popup_window_setup_nimf (PopupWindow *window,
                         NimfModule  *module)
{
	PopupWindowPrivate *priv = window->priv;

	if (MODULE_IS_NIMF (module)) {
		if (!priv->nimf_module) {
			priv->nimf_module = module;
			GtkWidget *w = nimf_module_control_new (priv->nimf_module, priv->size_group);
			gtk_widget_set_name (w, CONTROL_WIDGET_NAME);
			add_control_widget (window, w, CONTROL_TYPE_NIMF);

			g_signal_connect (G_OBJECT (w), "clicked",
					G_CALLBACK (on_nimf_button_clicked_cb), window);
		}
	}
}
