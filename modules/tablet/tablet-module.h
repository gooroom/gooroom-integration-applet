/* 
 *  Copyright (C) 2023 Gooroom <gooroom@gooroom.kr>
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

#ifndef __TABLET_MODULE_H__
#define __TABLET_MODULE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODULE_TYPE_TABLET            (tablet_module_get_type ())
#define TABLET_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODULE_TYPE_TABLET, TabletModule))
#define TABLET_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MODULE_TYPE_TABLET, TabletModuleClass))
#define MODULE_IS_TABLET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODULE_TYPE_TABLET))
#define MODULE_IS_TABLET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MODULE_TYPE_TABLET))
#define TABLET_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MODULE_TYPE_TABLET, TabletModuleClass))

typedef struct _TabletModule TabletModule;
typedef struct _TabletModuleClass TabletModuleClass;
typedef struct _TabletModulePrivate TabletModulePrivate;


struct _TabletModuleClass
{
	GObjectClass parent_class;

	/*< signals >*/
	void (*launch_command)(TabletModule *module, const gchar *command);
};

struct _TabletModule
{
	GObject parent;

	TabletModulePrivate *priv;
};

GType           tablet_module_get_type           (void) G_GNUC_CONST;

TabletModule   *tablet_module_new                (void);

GtkWidget      *tablet_module_control_new        (TabletModule *module,
                                                  GtkSizeGroup *size_group);
void            tablet_module_control_destroy    (TabletModule *module);

G_END_DECLS

#endif /* !__TABLET_MODULE_H__ */
