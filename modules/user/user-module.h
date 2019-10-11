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

#ifndef __USER_MODULE_H__
#define __USER_MODULE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODULE_TYPE_USER            (user_module_get_type ())
#define USER_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODULE_TYPE_USER, UserModule))
#define USER_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MODULE_TYPE_USER, UserModuleClass))
#define MODULE_IS_USER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODULE_TYPE_USER))
#define MODULE_IS_USER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MODULE_TYPE_USER))
#define USER_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MODULE_TYPE_USER, UserModuleClass))

typedef struct _UserModule UserModule;
typedef struct _UserModuleClass UserModuleClass;
typedef struct _UserModulePrivate UserModulePrivate;


struct _UserModuleClass
{
	GObjectClass parent_class;
};

struct _UserModule
{
	GObject parent;

	UserModulePrivate *priv;
};

GType        user_module_get_type         (void) G_GNUC_CONST;

UserModule  *user_module_new              (void);

GtkWidget   *user_module_tray_new         (UserModule   *module);
GtkWidget   *user_module_control_new      (UserModule   *module,
                                           GtkSizeGroup *size_group);

void         user_module_control_destroy  (UserModule   *module);

G_END_DECLS

#endif /* !__USER_MODULE_H__ */
