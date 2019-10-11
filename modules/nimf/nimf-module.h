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

#ifndef __NIMF_MODULE_H__
#define __NIMF_MODULE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODULE_TYPE_NIMF            (nimf_module_get_type ())
#define NIMF_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODULE_TYPE_NIMF, NimfModule))
#define NIMF_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MODULE_TYPE_NIMF, NimfModuleClass))
#define MODULE_IS_NIMF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODULE_TYPE_NIMF))
#define MODULE_IS_NIMF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MODULE_TYPE_NIMF))
#define NIMF_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MODULE_TYPE_NIMF, NimfModuleClass))

typedef struct _NimfModule NimfModule;
typedef struct _NimfModuleClass NimfModuleClass;
typedef struct _NimfModulePrivate NimfModulePrivate;


struct _NimfModuleClass
{
	GObjectClass parent_class;

	/*< signals >*/
	void (*launch_desktop)     (NimfModule *module, const gchar *desktop);

	void (*change_engine_done) (NimfModule *module);
};

struct _NimfModule
{
	GObject parent;

	NimfModulePrivate *priv;
};

GType           nimf_module_get_type             (void) G_GNUC_CONST;

NimfModule     *nimf_module_new                  (void);

GtkWidget      *nimf_module_tray_new             (NimfModule   *module);
GtkWidget      *nimf_module_control_new          (NimfModule   *module,
                                                  GtkSizeGroup *size_group);
GtkWidget      *nimf_module_control_menu_new     (NimfModule   *module);

void            nimf_module_control_destroy      (NimfModule   *module);
void            nimf_module_control_menu_destroy (NimfModule   *module);

G_END_DECLS

#endif /* !__NIMF_MODULE_H__ */
