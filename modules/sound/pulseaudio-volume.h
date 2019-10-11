/*  Copyright (c) 2014-2015 Andrzej <ndrwrdck@gmail.com>
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

#ifndef __PULSEAUDIO_VOLUME_H__
#define __PULSEAUDIO_VOLUME_H__

#include <glib-object.h>
//#include "pulseaudio-config.h"

G_BEGIN_DECLS

#define TYPE_PULSEAUDIO_VOLUME             (pulseaudio_volume_get_type ())
#define PULSEAUDIO_VOLUME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PULSEAUDIO_VOLUME, PulseaudioVolume))
#define PULSEAUDIO_VOLUME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_PULSEAUDIO_VOLUME, PulseaudioVolumeClass))
#define IS_PULSEAUDIO_VOLUME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PULSEAUDIO_VOLUME))
#define IS_PULSEAUDIO_VOLUME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_PULSEAUDIO_VOLUME))
#define PULSEAUDIO_VOLUME_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_PULSEAUDIO_VOLUME, PulseaudioVolumeClass))

typedef struct          _PulseaudioVolume                 PulseaudioVolume;
typedef struct          _PulseaudioVolumeClass            PulseaudioVolumeClass;

GType                   pulseaudio_volume_get_type        (void) G_GNUC_CONST;

PulseaudioVolume       *pulseaudio_volume_new             (void);

gboolean                pulseaudio_volume_get_connected   (PulseaudioVolume *volume);

gdouble                 pulseaudio_volume_get_volume      (PulseaudioVolume *volume);
void                    pulseaudio_volume_set_volume      (PulseaudioVolume *volume,
                                                           gdouble           vol);

gboolean                pulseaudio_volume_get_muted       (PulseaudioVolume *volume);
void                    pulseaudio_volume_set_muted       (PulseaudioVolume *volume,
                                                           gboolean          muted);
void                    pulseaudio_volume_toggle_muted    (PulseaudioVolume *volume);

G_END_DECLS

#endif /* !__PULSEAUDIO_VOLUME_H__ */
