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

#ifndef __POWER_MODULE_H__
#define __POWER_MODULE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODULE_TYPE_POWER            (power_module_get_type ())
#define POWER_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODULE_TYPE_POWER, PowerModule))
#define POWER_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MODULE_TYPE_POWER, PowerModuleClass))
#define MODULE_IS_POWER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODULE_TYPE_POWER))
#define MODULE_IS_POWER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MODULE_TYPE_POWER))
#define POWER_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MODULE_TYPE_POWER, PowerModuleClass))

typedef struct _PowerModule PowerModule;
typedef struct _PowerModuleClass PowerModuleClass;
typedef struct _PowerModulePrivate PowerModulePrivate;

struct _PowerModuleClass
{
	GObjectClass parent_class;

    /*< signals >*/
	void (*launch_desktop)(PowerModule *module, const gchar *desktop);
};

struct _PowerModule
{
	GObject parent;

	PowerModulePrivate *priv;
};

GType        power_module_get_type       (void) G_GNUC_CONST;

PowerModule *power_module_new            (void);

GtkWidget *power_module_tray_new                   (PowerModule  *module);
GtkWidget *power_module_brightness_control_new     (PowerModule  *module);
GtkWidget *power_module_battery_control_new        (PowerModule  *module);
void       power_module_battery_control_destroy    (PowerModule  *module);
void       power_module_brightness_control_destroy (PowerModule  *module);

G_END_DECLS

#endif /* !__POWER_MODULE_H__ */
