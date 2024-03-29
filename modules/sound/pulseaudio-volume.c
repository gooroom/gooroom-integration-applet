/*  
 *  Copyright (c) 2014-2015 Andrzej <ndrwrdck@gmail.com>
 *  Copyright (c) 2015-2021 Gooroom <gooroom@gooroom.co.kr>
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



/*
 *  This file implements a pulseaudio volume class abstracting out
 *  operations on pulseaudio mixer.
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>

//#include "pulseaudio-config.h"
#include "pulseaudio-debug.h"
#include "pulseaudio-volume.h"

//#define DEFAULT_ENABLE_KEYBOARD_SHORTCUTS         TRUE
//#define DEFAULT_SHOW_NOTIFICATIONS                TRUE
//#define DEFAULT_VOLUME_STEP                       6
#define DEFAULT_VOLUME_MAX                        153 /* 1 ~ 300 */


static void                 pulseaudio_volume_finalize           (GObject            *object);
static void                 pulseaudio_volume_connect            (PulseaudioVolume   *volume);
static gdouble              pulseaudio_volume_v2d                (PulseaudioVolume   *volume,
                                                                  pa_volume_t         vol);
static pa_volume_t          pulseaudio_volume_d2v                (PulseaudioVolume   *volume,
                                                                  gdouble             vol);
static gboolean             pulseaudio_volume_reconnect_timeout  (gpointer            userdata);


struct _PulseaudioVolume
{
  GObject               __parent__;

//  PulseaudioConfig     *config;

  pa_glib_mainloop     *pa_mainloop;
  pa_context           *pa_context;
  gboolean              connected;

  gdouble               volume;
  gboolean              muted;

  gdouble               volume_mic;
  gboolean              muted_mic;

  guint                 reconnect_timer_id;
};

struct _PulseaudioVolumeClass
{
  GObjectClass          __parent__;
};




enum
{
  VOLUME_CHANGED,
  LAST_SIGNAL
};

static guint pulseaudio_volume_signals[LAST_SIGNAL] = { 0, };




G_DEFINE_TYPE (PulseaudioVolume, pulseaudio_volume, G_TYPE_OBJECT)

static void
pulseaudio_volume_class_init (PulseaudioVolumeClass *klass)
{
  GObjectClass      *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = pulseaudio_volume_finalize;

  pulseaudio_volume_signals[VOLUME_CHANGED] =
    g_signal_new (g_intern_static_string ("volume-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

}



static void
pulseaudio_volume_init (PulseaudioVolume *volume)
{
  volume->connected = FALSE;
  volume->volume = 0.0;
  volume->muted = FALSE;
  volume->reconnect_timer_id = 0;

  volume->pa_mainloop = pa_glib_mainloop_new (NULL);

  pulseaudio_volume_connect (volume);
}



static void
pulseaudio_volume_finalize (GObject *object)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (object);

//  volume->config = NULL;

  pa_glib_mainloop_free (volume->pa_mainloop);

  (*G_OBJECT_CLASS (pulseaudio_volume_parent_class)->finalize) (object);
}




/* sink event callbacks */
static void
pulseaudio_volume_sink_info_cb (pa_context         *context,
                                const pa_sink_info *i,
                                int                 eol,
                                void               *userdata)
{
  int c;
  gboolean  muted;
  gdouble   vol;
  gdouble   vol_max = 0.0;

  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;
  pulseaudio_debug ("sink info: %s, %s", i->name, i->description);

  for (c = 0; c < i->volume.channels; c++) {
    vol_max = MAX (vol_max, (gdouble)i->volume.values[c]);
  }

  muted = !!(i->mute);
  vol = pulseaudio_volume_v2d (volume, vol_max);

  if (volume->muted != muted)
    {
      pulseaudio_debug ("Updated Mute: %d -> %d", volume->muted, muted);
      volume->muted = muted;
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0);
    }

  if (ABS (volume->volume - vol) > 2e-3)
    {
      pulseaudio_debug ("Updated Volume: %04.3f -> %04.3f", volume->volume, vol);
      volume->volume = vol;
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0);
    }
  pulseaudio_debug ("volume: %f, muted: %d", vol, muted);
}



static void
pulseaudio_volume_server_info_cb (pa_context           *context,
                                  const pa_server_info *i,
                                  void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  pulseaudio_debug ("server: %s@%s, v.%s", i->user_name, i->server_name, i->server_version);
  pa_context_get_sink_info_by_name (context, i->default_sink_name, pulseaudio_volume_sink_info_cb, volume);
}




static void
pulseaudio_volume_sink_check (PulseaudioVolume *volume,
                              pa_context       *context)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));

  pa_context_get_server_info (context, pulseaudio_volume_server_info_cb, volume);
}




static void
pulseaudio_volume_subscribe_cb (pa_context                   *context,
                                pa_subscription_event_type_t  t,
                                uint32_t                      idx,
                                void                         *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)
    {
    case PA_SUBSCRIPTION_EVENT_SINK          :
      pulseaudio_volume_sink_check (volume, context);
      pulseaudio_debug ("received sink event");
      break;

    case PA_SUBSCRIPTION_EVENT_SOURCE        :
      pulseaudio_debug ("received source event");
      break;

    case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT :
      pulseaudio_debug ("received source output event");
      break;

    default                                  :
      pulseaudio_debug ("received unknown pulseaudio event");
      break;
    }
}




static void
pulseaudio_volume_context_state_cb (pa_context *context,
                                    void       *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  switch (pa_context_get_state (context))
    {
    case PA_CONTEXT_READY        :
      pa_context_subscribe (context, PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT, NULL, NULL);
      pa_context_set_subscribe_callback (context, pulseaudio_volume_subscribe_cb, volume);

      pulseaudio_debug ("PulseAudio connection established");
      volume->connected = TRUE;
      // Check current sink volume manually. PA sink events usually not emitted.
      pulseaudio_volume_sink_check (volume, context);
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0);
      break;

    case PA_CONTEXT_FAILED       :
    case PA_CONTEXT_TERMINATED   :
      g_warning ("Disconected from the PulseAudio server. Attempting to reconnect in 5 seconds.");
      volume->pa_context = NULL;
      volume->connected = FALSE;
      volume->volume = 0.0;
      volume->muted = FALSE;
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0);
      if (volume->reconnect_timer_id == 0)
        volume->reconnect_timer_id = g_timeout_add_seconds
          (5, pulseaudio_volume_reconnect_timeout, volume);
      break;

    case PA_CONTEXT_CONNECTING   :
      pulseaudio_debug ("Connecting to PulseAudio server");
      break;

    case PA_CONTEXT_SETTING_NAME :
      pulseaudio_debug ("Setting application name");
      break;

    case PA_CONTEXT_AUTHORIZING  :
      pulseaudio_debug ("Authorizing");
      break;

    case PA_CONTEXT_UNCONNECTED  :
      pulseaudio_debug ("Not connected to PulseAudio server");
      break;

    default                      :
      g_warning ("Unknown pulseaudio context state");
      break;
    }
}



static void
pulseaudio_volume_connect (PulseaudioVolume *volume)
{
  pa_proplist  *proplist;
  gint          err;

  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (!volume->connected);

  proplist = pa_proplist_new ();
#ifdef HAVE_CONFIG_H
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_NAME, PACKAGE_NAME);
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_VERSION, PACKAGE_VERSION);
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_ID, "kr.gooroom.IntegrationApplet.modules.sound");
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
#endif

  volume->pa_context = pa_context_new_with_proplist (pa_glib_mainloop_get_api (volume->pa_mainloop), NULL, proplist);
  pa_context_set_state_callback(volume->pa_context, pulseaudio_volume_context_state_cb, volume);

  err = pa_context_connect (volume->pa_context, NULL, PA_CONTEXT_NOFAIL, NULL);
  if (err < 0)
    g_warning ("pa_context_connect() failed: %s", pa_strerror (err));
}




static gboolean
pulseaudio_volume_reconnect_timeout  (gpointer userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  volume->reconnect_timer_id = 0;
  pulseaudio_volume_connect (volume);

  return FALSE; // stop the timer
}





gboolean
pulseaudio_volume_get_connected (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), FALSE);

  return volume->connected;
}



static gdouble
pulseaudio_volume_v2d (PulseaudioVolume *volume,
                       pa_volume_t       pa_volume)
{
  gdouble vol;
  gdouble vol_max;

  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), 0.0);

//  vol_max = pulseaudio_config_get_volume_max (volume->config) / 100.0;
  vol_max = DEFAULT_VOLUME_MAX / 100.0;

  vol = (gdouble) pa_volume - PA_VOLUME_MUTED;
  vol /= (gdouble) (PA_VOLUME_NORM - PA_VOLUME_MUTED);
  /* for safety */
  vol = MIN (MAX (vol, 0.0), vol_max);
  return vol;
}



static pa_volume_t
pulseaudio_volume_d2v (PulseaudioVolume *volume,
                       gdouble           vol)
{
  gdouble pa_volume;

  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), PA_VOLUME_MUTED);

  pa_volume = (PA_VOLUME_NORM - PA_VOLUME_MUTED) * vol;
  pa_volume = (pa_volume_t) pa_volume + PA_VOLUME_MUTED;
  /* for safety */
  pa_volume = MIN (MAX (pa_volume, PA_VOLUME_MUTED), PA_VOLUME_MAX);
  return pa_volume;
}




gboolean
pulseaudio_volume_get_muted (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), FALSE);

  return volume->muted;
}



/* final callback for volume/mute changes */
/* pa_context_success_cb_t */
static void
pulseaudio_volume_sink_volume_changed (pa_context *context,
                                       int         success,
                                       void       *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (success)
    g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0);
}

/* mute setting callbacks */
/* pa_sink_info_cb_t */
static void
pulseaudio_volume_set_muted_cb1 (pa_context         *context,
                                 const pa_sink_info *i,
                                 int                 eol,
                                 void               *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  pa_context_set_sink_mute_by_index (context, i->index, volume->muted, pulseaudio_volume_sink_volume_changed, volume);
}



void
pulseaudio_volume_set_muted (PulseaudioVolume *volume,
                             gboolean          muted)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (volume->pa_context != NULL);
  g_return_if_fail (pa_context_get_state (volume->pa_context) == PA_CONTEXT_READY);

  if (volume->muted != muted)
    {
      volume->muted = muted;
      pa_context_get_sink_info_list (volume->pa_context, pulseaudio_volume_set_muted_cb1, volume);
    }
}



void
pulseaudio_volume_toggle_muted (PulseaudioVolume *volume)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));

  pulseaudio_volume_set_muted (volume, !volume->muted);
}




gdouble
pulseaudio_volume_get_volume (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), 0.0);

  return volume->volume;
}



/* volume setting callbacks */
/* pa_sink_info_cb_t */
static void
pulseaudio_volume_set_volume_cb2 (pa_context         *context,
                                  const pa_sink_info *i,
                                  int                 eol,
                                  void               *userdata)
{
  //char st[PA_CVOLUME_SNPRINT_MAX];

  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  //pulseaudio_debug ("*** %s", pa_cvolume_snprint (st, sizeof (st), &i->volume));
  pa_cvolume_set ((pa_cvolume *)&i->volume, 1, pulseaudio_volume_d2v (volume, volume->volume));
  pa_context_set_sink_volume_by_index (context, i->index, &i->volume, pulseaudio_volume_sink_volume_changed, volume);
}



/* pa_server_info_cb_t */
static void
pulseaudio_volume_set_volume_cb1 (pa_context           *context,
                                  const pa_server_info *i,
                                  void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  pa_context_get_sink_info_by_name (context, i->default_sink_name, pulseaudio_volume_set_volume_cb2, volume);
}


void
pulseaudio_volume_set_volume (PulseaudioVolume *volume,
                              gdouble           vol)
{
  gdouble vol_max;
  gdouble vol_trim;

  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (volume->pa_context != NULL);
  g_return_if_fail (pa_context_get_state (volume->pa_context) == PA_CONTEXT_READY);

//  vol_max = pulseaudio_config_get_volume_max (volume->config) / 100.0;
  vol_max = DEFAULT_VOLUME_MAX / 100.0;
  vol_trim = MIN (MAX (vol, 0.0), vol_max);

  if (volume->volume != vol_trim)
    {
      volume->volume = vol_trim;
      pa_context_get_server_info (volume->pa_context, pulseaudio_volume_set_volume_cb1, volume);
    }
}



PulseaudioVolume *
pulseaudio_volume_new (void)
{
  PulseaudioVolume *volume;

//  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), NULL);

  volume = g_object_new (TYPE_PULSEAUDIO_VOLUME, NULL);
//  volume->config = config;

  return volume;
}


