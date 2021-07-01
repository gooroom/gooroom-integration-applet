/*
 * Copyright (C) 2015-2021 Gooroom <gooroom@gooroom.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef _COMMON_H_
#define _COMMON_H_


#include <glib.h>
#include <gtk/gtk.h>

#include "panel-glib.h"

G_BEGIN_DECLS

#define TRAY_ICON_SIZE                         (24)
#define STATUS_ICON_SIZE                       (24)
#define GOOROOM_MANAGEMENT_SERVER_CONF         "/etc/gooroom/gooroom-client-server-register/gcsr.conf"
#define GOOROOM_AGENT_SERVICE_NAME             "gooroom-agent.service"

gboolean is_local_user                     (void);
gboolean is_admin_group                    (void);
gboolean is_standalone_mode                (void);

gboolean authenticate                      (const gchar *action_id);

gboolean is_systemd_service_active         (const gchar *service_name);

gboolean is_systemd_service_available      (const gchar *service_name);

gboolean panel_app_info_launch_uris        (GAppInfo   *appinfo,
                                            GList      *uris,
                                            GdkScreen  *screen,
                                            guint32     timestamp,
                                            GError    **error);

gboolean launch_desktop_id                 (const char  *desktop_id,
                                            GdkScreen   *screen);


G_END_DECLS


#endif /* _COMMON_H_ */
