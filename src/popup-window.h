/*-
 * Copyright (C) 2015-2019 Gooroom <gooroom@gooroom.kr>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __POPUP_WINDOW_H__
#define __POPUP_WINDOW_H__

#include <gtk/gtk.h>

#include "modules/user/user-module.h"
#include "modules/sound/sound-module.h"
#include "modules/power/power-module.h"
#include "modules/datetime/datetime-module.h"
#include "modules/security/security-module.h"
#include "modules/endsession/endsession-module.h"
#include "modules/nimf/nimf-module.h"
#include "modules/security/utils/utils.h"

G_BEGIN_DECLS

#define WINDOW_TYPE_POPUP            (popup_window_get_type ())
#define POPUP_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_TYPE_POPUP, PopupWindow))
#define POPUP_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_TYPE_POPUP, PopupWindowClass))
#define WINDOW_IS_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_TYPE_POPUP))
#define WINDOW_IS_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_TYPE_POPUP))
#define POPUP_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_TYPE_POPUP, PopupWindowClass))

typedef struct _PopupWindowPrivate  PopupWindowPrivate;
typedef struct _PopupWindowClass    PopupWindowClass;
typedef struct _PopupWindow         PopupWindow;

enum {
	POPUP_WINDOW_CLOSED = 1
};


struct _PopupWindowClass
{
	GtkWindowClass __parent__;

	/*< signals >*/
	void (*closed)(PopupWindow *window, gint reason);

	void (*launch_desktop)(PopupWindow *window, const gchar *desktop);
	void (*launch_command)(PopupWindow *window, const gchar *command);

};

struct _PopupWindow
{
	GtkWindow __parent__;

	PopupWindowPrivate *priv;
};


GType        popup_window_get_type (void) G_GNUC_CONST;

PopupWindow *popup_window_new      (GtkWidget *parent);

void popup_window_set_workarea     (PopupWindow  *window,
                                    GdkRectangle *workarea);

void popup_window_setup_user       (PopupWindow *window,
                                    UserModule  *module);

void popup_window_setup_power      (PopupWindow *window,
                                    PowerModule *module);

void popup_window_setup_sound      (PopupWindow *window,
                                    SoundModule *module);

void popup_window_setup_security   (PopupWindow    *window,
                                    SecurityModule *module);

void popup_window_setup_datetime   (PopupWindow    *window,
                                    DateTimeModule *module);

void popup_window_setup_endsession (PopupWindow      *window,
                                    EndSessionModule *module);

void popup_window_setup_nimf       (PopupWindow *window,
                                    NimfModule  *module);

G_END_DECLS

#endif /* !__POPUP_WINDOW_H__ */
