/*
 * Origianl work Copyright (c) 2014-2017 Andrzej <andrzejr@xfce.org>
 *                             2017      Viktor Odintsev <zakhams@gmail.com>
 *                             2017      Matthieu Mota <matthieumota@gmail.com>
 *
 * Modified work Copyright (c) 2015-2021 Gooroom <gooroom@gooroom.kr>
 *
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


#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <canberra-gtk.h>

#include "common.h"
#include "sound-module.h"

#include "pulseaudio-volume.h"

//#define VOLUME_PLUGIN_RAISE_VOLUME_KEY  "XF86AudioRaiseVolume"
//#define VOLUME_PLUGIN_LOWER_VOLUME_KEY  "XF86AudioLowerVolume"
//#define VOLUME_PLUGIN_MUTE_KEY          "XF86AudioMute"




struct _SoundModulePrivate
{
	PulseaudioVolume  *volume;

	GtkWidget         *tray;
	GtkWidget         *scale;
	GtkWidget         *control;
	GtkWidget         *status_icon;
	GtkWidget         *status_button;

//	NotifyNotification* notification;
};



G_DEFINE_TYPE_WITH_PRIVATE (SoundModule, sound_module, G_TYPE_OBJECT)


#if 0
static void
notify_notification (NotifyNotification *notification,
                     const gchar        *icon,
                     gint                value)
{
    GdkPixbuf *pix = NULL;

	pix = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                    icon, 32,
                                    GTK_ICON_LOOKUP_FORCE_SIZE,
                                    NULL);

	notify_notification_set_image_from_pixbuf (notification, pix);
	notify_notification_set_hint_int32 (notification, "value", value);
	notify_notification_show (notification, NULL);
}

static void
notify_volume_notification (NotifyNotification *notification,
                            gboolean            muted,
                            gdouble             volume)
{
	const gchar *icon;

	if (muted) {
		icon = "audio-volume-muted";
	} else {
		if (volume <= 0.0)
			icon = "audio-volume-zero";
		else if (volume <= 0.3)
			icon = "audio-volume-low";
		else if (volume <= 0.7)
			icon = "audio-volume-medium";
		else
			icon = "audio-volume-high";
	}

	notify_notification (notification, icon, volume);
}

static void
sound_module_volume_key_pressed (const char *keystring, gpointer data)
{
	SoundModule *module      = VOLUME_PLUGIN (data);
	gdouble       volume      = pulseaudio_volume_get_volume (module->volume);
	gdouble       volume_step = pulseaudio_config_get_volume_step (module->config) / 100.0;
	gboolean      muted       = pulseaudio_volume_get_muted (module->volume);
	gdouble new_volume;

	if (muted) {
		new_volume = volume;
	} else {
		if (g_strcmp0 (keystring, VOLUME_PLUGIN_RAISE_VOLUME_KEY) == 0)
			new_volume = MIN (volume + volume_step, MAX (volume, 1.0));
		else if (strcmp (keystring, VOLUME_PLUGIN_LOWER_VOLUME_KEY) == 0)
			new_volume = volume - volume_step;
		else
			return;
	}

	/* Play a sound! */
	ca_gtk_play_for_widget (GTK_WIDGET (plugin), 0,
			CA_PROP_EVENT_ID, "audio-volume-change",
			NULL);

	pulseaudio_volume_set_volume (module->volume, new_volume);

	notify_volume_notification (module->notification, muted, new_volume * 100.0);
}

static void
sound_module_mute_pressed (const char *keystring, gpointer data)
{
	SoundModule *module = VOLUME_PLUGIN (data);

	gdouble volume = pulseaudio_volume_get_volume (module->volume);
	gboolean muted = pulseaudio_volume_get_muted (module->volume);

	pulseaudio_volume_set_muted (module->volume, !muted);

	/* Play a sound! */
	ca_gtk_play_for_widget (GTK_WIDGET (plugin), 0,
			CA_PROP_EVENT_ID, "audio-volume-change",
			NULL);

	notify_volume_notification (module->notification, !muted, volume * 100.0);

	pulseaudio_volume_toggle_muted (module->volume);
}

static gboolean
sound_module_bind_keys (SoundModule *module)
{
	gboolean success;

	success = (keybinder_bind (VOLUME_PLUGIN_LOWER_VOLUME_KEY, sound_module_volume_key_pressed, module) &&
			keybinder_bind (VOLUME_PLUGIN_RAISE_VOLUME_KEY, sound_module_volume_key_pressed, module) &&
			keybinder_bind (VOLUME_PLUGIN_MUTE_KEY, sound_module_mute_pressed, module));

	return success;
}

static void
sound_module_unbind_keys (SoundModule *module)
{
	keybinder_unbind (VOLUME_PLUGIN_LOWER_VOLUME_KEY, sound_module_volume_key_pressed);
	keybinder_unbind (VOLUME_PLUGIN_RAISE_VOLUME_KEY, sound_module_volume_key_pressed);
	keybinder_unbind (VOLUME_PLUGIN_MUTE_KEY, sound_module_mute_pressed);
}

static void
sound_module_bind_keys_cb (PulseaudioConfig *config, gpointer data)
{
	SoundModule *module = SOUND_MODULE (data);

	if (pulseaudio_config_get_enable_keyboard_shortcuts (config))
		sound_module_bind_keys (module);
	else
		sound_module_unbind_keys (module);
}
#endif

static void
tray_icon_update (SoundModule *module)
{
	SoundModulePrivate *priv = module->priv;

	if (!module || !priv->tray)
		return;

	const gchar *icon_name;
	gboolean connected = FALSE;

	connected = pulseaudio_volume_get_connected (priv->volume);

	if (connected) {
		gboolean muted = pulseaudio_volume_get_muted (priv->volume);
		gdouble volume = pulseaudio_volume_get_volume (priv->volume);
		if (muted) {
			icon_name = "audio-volume-muted-symbolic";
		} else {
			if (volume <= 0.0)
				icon_name = "audio-volume-zero-symbolic";
			else if (volume <= 0.3)
				icon_name = "audio-volume-low-symbolic";
			else if (volume <= 0.7)
				icon_name = "audio-volume-medium-symbolic";
			else
				icon_name = "audio-volume-high-symbolic";
		}
	} else {
		icon_name = "audio-volume-error-symbolic";
	}

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->tray), icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
}

static void
on_scale_value_changed (GtkRange *range, gpointer data)
{
	SoundModule *module = SOUND_MODULE (data);
	SoundModulePrivate *priv = module->priv;

//	if (module->notification)
//		notify_notification_close (module->notification, NULL);

	gdouble new_volume = gtk_range_get_value (GTK_RANGE (range));
	pulseaudio_volume_set_volume (priv->volume, new_volume / 100.0);
}

static void
sync_volume_control (SoundModule *module)
{
	SoundModulePrivate *priv = module->priv;

	if (!module || !priv->control)
		return;

	const gchar *icon_name;
	gboolean connected = FALSE;

	connected = pulseaudio_volume_get_connected (priv->volume);

	if (connected) {
		gboolean muted = pulseaudio_volume_get_muted (priv->volume);
		gdouble volume = pulseaudio_volume_get_volume (priv->volume);
		if (muted) {
			icon_name = "audio-volume-muted-symbolic";
		} else {
			if (volume <= 0.0)
				icon_name = "audio-volume-zero-symbolic";
			else if (volume <= 0.3)
				icon_name = "audio-volume-low-symbolic";
			else if (volume <= 0.7)
				icon_name = "audio-volume-medium-symbolic";
			else
				icon_name = "audio-volume-high-symbolic";
		}
		gtk_widget_set_sensitive (priv->scale, !muted);
		gtk_widget_set_sensitive (priv->status_button, TRUE);

		g_signal_handlers_block_by_func (G_OBJECT (priv->scale), on_scale_value_changed, module);
		gtk_range_set_value (GTK_RANGE (priv->scale), volume * 100.0);
		g_signal_handlers_unblock_by_func (G_OBJECT (priv->scale), on_scale_value_changed, module);
	} else {
		icon_name = "audio-volume-error-symbolic";

		gtk_widget_set_sensitive (priv->scale, FALSE);
		gtk_widget_set_sensitive (priv->status_button, FALSE);

		gtk_range_set_value (GTK_RANGE (priv->scale), 0);
	}

	gtk_image_set_from_icon_name (GTK_IMAGE (priv->status_icon), icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_pixel_size (GTK_IMAGE (priv->status_icon), STATUS_ICON_SIZE);
}

static void
on_mute_button_clicked (GtkToggleButton *button, gpointer data)
{
	SoundModule *module = SOUND_MODULE (data);
	SoundModulePrivate *priv = module->priv;

	gboolean muted = pulseaudio_volume_get_muted (priv->volume);

	pulseaudio_volume_set_muted (priv->volume, !muted);

	/* Play a sound! */
	ca_gtk_play_for_event (gtk_get_current_event (), 0,
			CA_PROP_EVENT_ID, "audio-volume-change",
			NULL);
}

static gboolean
on_scale_button_release_event (GtkWidget      *widget,
                               GdkEventButton *event,
                               gpointer        data)
{
	/* Play a sound! */
	ca_gtk_play_for_event (gtk_get_current_event (), 0,
			CA_PROP_EVENT_ID, "audio-volume-change",
			NULL);

	return FALSE;
}

static void
on_volume_changed (PulseaudioVolume *volume, gpointer data)
{
	SoundModule *module = SOUND_MODULE (data);

	tray_icon_update (module);

	sync_volume_control (module);
}

static gboolean
tray_icon_update_delay (gpointer data)
{
	SoundModule *module = SOUND_MODULE (data);

#if 0
	notify_init ("audio-volume-notify");

	module->notification = notify_notification_new ("audio-volumed-notify", NULL, NULL);

	/* Initialize libkeybinder */
	keybinder_init ();

	g_signal_connect (G_OBJECT (module->config), "notify::enable-keyboard-shortcuts", G_CALLBACK (sound_module_bind_keys_cb), module);
	if (pulseaudio_config_get_enable_keyboard_shortcuts (module->config))
		sound_module_bind_keys (module);
	else
		sound_module_unbind_keys (module);
#endif

	tray_icon_update (module);

	return FALSE;
}

static void
build_control_ui (SoundModule *module)
{
	GtkWidget *scale, *icon;
	SoundModulePrivate *priv = module->priv;
	GtkStyleContext *context;

	priv->control = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 15);
	gtk_container_set_border_width (GTK_CONTAINER (priv->control), 0);

	priv->status_button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (priv->status_button), GTK_RELIEF_NONE);
	gtk_widget_set_valign (priv->status_button, GTK_ALIGN_CENTER);
	gtk_widget_set_halign (priv->status_button, GTK_ALIGN_CENTER);
	gtk_box_pack_start (GTK_BOX (priv->control), priv->status_button, FALSE, FALSE, 0);

	context = gtk_widget_get_style_context (priv->status_button);
	gtk_style_context_add_class (context, "rounded-icon-button");

	g_signal_connect (G_OBJECT (priv->status_button), "clicked", G_CALLBACK (on_mute_button_clicked), module);

	priv->status_icon = icon = gtk_image_new_from_icon_name ("audio-volume-muted-symbolic",
                                                             GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_image_set_pixel_size (GTK_IMAGE (icon), STATUS_ICON_SIZE);
	gtk_widget_set_valign (icon, GTK_ALIGN_CENTER);
	gtk_widget_set_halign (icon, GTK_ALIGN_CENTER);
	gtk_container_add (GTK_CONTAINER (priv->status_button), icon);

	priv->scale = scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
	gtk_widget_set_can_focus (scale, FALSE);
	gtk_range_set_inverted (GTK_RANGE (scale), FALSE);
	gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
	gtk_range_set_round_digits (GTK_RANGE (scale), 0);
	gtk_box_pack_end (GTK_BOX (priv->control), scale, TRUE, TRUE, 0);

	g_signal_connect (G_OBJECT (scale), "value-changed", G_CALLBACK (on_scale_value_changed), module);
	g_signal_connect (G_OBJECT (scale), "button-release-event", G_CALLBACK (on_scale_button_release_event), module);
}

static void
sound_module_finalize (GObject *object)
{
	SoundModule *module = SOUND_MODULE (object);

	sound_module_control_destroy (module);

	G_OBJECT_CLASS (sound_module_parent_class)->finalize (object);
}

static void
sound_module_init (SoundModule *module)
{
	SoundModulePrivate *priv;

	module->priv = priv = sound_module_get_instance_private (module);

	priv->volume        = NULL;
	priv->tray          = NULL;
	priv->scale         = NULL;
	priv->control       = NULL;
	priv->status_icon   = NULL;
	priv->status_button = NULL;
//	priv->notification   = NULL;

	priv->volume = pulseaudio_volume_new ();

	g_signal_connect (G_OBJECT (priv->volume), "volume-changed", G_CALLBACK (on_volume_changed), module);
}

static void
sound_module_class_init (SoundModuleClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);

	object_class->finalize = sound_module_finalize;
}

SoundModule *
sound_module_new (void)
{
	return g_object_new (MODULE_TYPE_SOUND, NULL);
}


GtkWidget *
sound_module_tray_new (SoundModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	SoundModulePrivate *priv = module->priv;

	if (!priv->tray) {
		priv->tray = gtk_image_new_from_icon_name ("audio-volume-high-symbolic",
                                                   GTK_ICON_SIZE_LARGE_TOOLBAR);
		gtk_image_set_pixel_size (GTK_IMAGE (priv->tray), TRAY_ICON_SIZE);
	}

	gtk_widget_show_all (priv->tray);

	g_idle_add ((GSourceFunc) tray_icon_update_delay, module);

	return priv->tray;
}

GtkWidget *
sound_module_control_new (SoundModule *module)
{
	g_return_val_if_fail (module != NULL, NULL);

	SoundModulePrivate *priv = module->priv;

	build_control_ui (module);

	gtk_widget_show_all (priv->control);

	sync_volume_control (module);

	return priv->control;
}

void
sound_module_control_destroy (SoundModule *module)
{
	g_return_if_fail (module != NULL);

	SoundModulePrivate *priv = module->priv;

	if (priv->control) {
		gtk_widget_destroy (priv->control);
		priv->control = NULL;
	}
}
