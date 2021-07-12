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

#ifndef __UPDATER_MODULE_H__
#define __UPDATER_MODULE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODULE_TYPE_UPDATER            (updater_module_get_type ())
#define UPDATER_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODULE_TYPE_UPDATER, UpdaterModule))
#define UPDATER_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MODULE_TYPE_UPDATER, UpdaterModuleClass))
#define MODULE_IS_UPDATER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODULE_TYPE_UPDATER))
#define MODULE_IS_UPDATER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MODULE_TYPE_UPDATER))
#define UPDATER_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MODULE_TYPE_UPDATER, UpdaterModuleClass))

typedef struct _UpdaterModule UpdaterModule;
typedef struct _UpdaterModuleClass UpdaterModuleClass;
typedef struct _UpdaterModulePrivate UpdaterModulePrivate;

struct _UpdaterModuleClass
{
	GObjectClass parent_class;

	/*< signals >*/
	void (*destroy_popup)(UpdaterModule *module);
	void (*launch_desktop)(UpdaterModule *module, const gchar *desktop);
};

struct _UpdaterModule
{
	GObject parent;

	UpdaterModulePrivate *priv;
};

GType          updater_module_get_type             (void) G_GNUC_CONST;

UpdaterModule *updater_module_new                  (void);

GtkWidget     *updater_module_tray_new             (UpdaterModule *module);
GtkWidget     *updater_module_control_new          (UpdaterModule *module);
GtkWidget     *updater_module_control_menu_new     (UpdaterModule *module);
void           updater_module_control_destroy      (UpdaterModule *module);
void           updater_module_control_menu_destroy (UpdaterModule *module);


G_END_DECLS

#endif /* !__UPDATER_MODULE_H__ */
