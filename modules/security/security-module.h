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

#ifndef __SECURITY_MODULE_H__
#define __SECURITY_MODULE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODULE_TYPE_SECURITY          (security_module_get_type ())
#define SECURITY_MODULE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODULE_TYPE_SECURITY, SecurityModule))
#define SECURITY_MODULE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), MODULE_TYPE_SECURITY, SecurityModuleClass))
#define MODULE_IS_SECURITY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODULE_TYPE_SECURITY))
#define MODULE_IS_SECURITY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MODULE_TYPE_SECURITY))
#define SECURITY_MODULE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MODULE_TYPE_SECURITY, SecurityModuleClass))

typedef struct _SecurityModule SecurityModule;
typedef struct _SecurityModuleClass SecurityModuleClass;
typedef struct _SecurityModulePrivate SecurityModulePrivate;


struct _SecurityModuleClass
{
	GObjectClass parent_class;

	/*< signals >*/
	void (*launch_desktop)(SecurityModule *module, const gchar *desktop);
};

struct _SecurityModule
{
	GObject parent;

	SecurityModulePrivate *priv;
};

GType           security_module_get_type             (void) G_GNUC_CONST;

SecurityModule *security_module_new                  (void);

GtkWidget      *security_module_tray_new             (SecurityModule *module);
GtkWidget      *security_module_control_new          (SecurityModule *module,
                                                      GtkSizeGroup   *size_group);
GtkWidget      *security_module_control_menu_new     (SecurityModule *module);

void            security_module_control_destroy      (SecurityModule *module);
void            security_module_control_menu_destroy (SecurityModule *module);
gchar *         get_lbl_sec_status (gpointer data);
guint           last_vulnerable_get (void);

G_END_DECLS

#endif /* !__SECURITY_MODULE_H__ */
