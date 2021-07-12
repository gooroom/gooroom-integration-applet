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

#ifndef __ENDSESSION_MODULE_H__
#define __ENDSESSION_MODULE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MODULE_TYPE_ENDSESSION          (endsession_module_get_type ())
#define ENDSESSION_MODULE(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), MODULE_TYPE_ENDSESSION, EndSessionModule))
#define ENDSESSION_MODULE_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST ((klass), MODULE_TYPE_ENDSESSION, EndSessionModuleClass))
#define MODULE_IS_ENDSESSION(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MODULE_TYPE_ENDSESSION))
#define MODULE_IS_ENDSESSION_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MODULE_TYPE_ENDSESSION))
#define ENDSESSION_MODULE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), MODULE_TYPE_ENDSESSION, EndSessionModuleClass))

typedef struct _EndSessionModule EndSessionModule;
typedef struct _EndSessionModuleClass EndSessionModuleClass;
typedef struct _EndSessionModulePrivate EndSessionModulePrivate;

struct _EndSessionModuleClass
{
	GObjectClass parent_class;

	/*< signals >*/
	void (*launch_command)(EndSessionModule *module, const gchar *command);
};

struct _EndSessionModule
{
	GObject parent;

	EndSessionModulePrivate *priv;
};

GType             endsession_module_get_type         (void) G_GNUC_CONST;

EndSessionModule *endsession_module_new              (void);

GtkWidget        *endsession_module_control_new      (EndSessionModule   *module);
void              endsession_module_control_destroy  (EndSessionModule   *module);

G_END_DECLS

#endif /* !__ENDSESSION_MODULE_H__ */
