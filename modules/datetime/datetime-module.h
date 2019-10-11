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

#ifndef __DATETIME_MODULE_H__
#define __DATETIME_MODULE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODULE_TYPE_DATETIME          (datetime_module_get_type ())
#define DATETIME_MODULE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODULE_TYPE_DATETIME, DateTimeModule))
#define DATETIME_MODULE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), MODULE_TYPE_DATETIME, DateTimeModuleClass))
#define MODULE_IS_DATETIME(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODULE_TYPE_DATETIME))
#define MODULE_IS_DATETIME_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MODULE_TYPE_DATETIME))
#define DATETIME_MODULE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MODULE_TYPE_DATETIME, DateTimeModuleClass))

typedef struct _DateTimeModule DateTimeModule;
typedef struct _DateTimeModuleClass DateTimeModuleClass;
typedef struct _DateTimeModulePrivate DateTimeModulePrivate;

struct _DateTimeModuleClass
{
	GObjectClass parent_class;

	/*< signals >*/
	void (*launch_desktop)(DateTimeModule *module, const gchar *desktop);
};

struct _DateTimeModule
{
	GObject parent;

	DateTimeModulePrivate *priv;
};

GType           datetime_module_get_type             (void) G_GNUC_CONST;

DateTimeModule *datetime_module_new                  (void);

GtkWidget      *datetime_module_tray_new             (DateTimeModule *module);
GtkWidget      *datetime_module_control_new          (DateTimeModule *module,
                                                      GtkSizeGroup   *size_group);
GtkWidget      *datetime_module_control_menu_new     (DateTimeModule *module);

void            datetime_module_control_destroy      (DateTimeModule *module);
void            datetime_module_control_menu_destroy (DateTimeModule *module);

G_END_DECLS

#endif /* !__DATETIME_MODULE_H__ */
