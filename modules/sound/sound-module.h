/* 
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

#ifndef __SOUND_MODULE_H__
#define __SOUND_MODULE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODULE_TYPE_SOUND          (sound_module_get_type ())
#define SOUND_MODULE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODULE_TYPE_SOUND, SoundModule))
#define SOUND_MODULE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), MODULE_TYPE_SOUND, SoundModuleClass))
#define MODULE_IS_SOUND(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODULE_TYPE_SOUND))
#define MODULE_IS_SOUND_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MODULE_TYPE_SOUND))
#define SOUND_MODULE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MODULE_TYPE_SOUND, SoundModuleClass))

typedef struct _SoundModule SoundModule;
typedef struct _SoundModuleClass SoundModuleClass;
typedef struct _SoundModulePrivate SoundModulePrivate;


struct _SoundModule
{
  GObject parent;

  SoundModulePrivate *priv;
};

struct _SoundModuleClass
{
	GObjectClass parent_class;
};

GType        sound_module_get_type        (void) G_GNUC_CONST;

SoundModule *sound_module_new             (void);

GtkWidget   *sound_module_tray_new        (SoundModule  *module);
GtkWidget   *sound_module_control_new     (SoundModule  *module);
void         sound_module_control_destroy (SoundModule  *module);

G_END_DECLS

#endif /* !__SOUND_MODULE_H__ */
