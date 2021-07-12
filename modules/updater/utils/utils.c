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
 *
 */


#include "utils.h"

#include <glib.h>
#include <gio/gio.h>

static void
update_operation_cb (GDBusProxy  *proxy,
                     const gchar *sender_name,
                     const gchar *signal_name,
                     GVariant    *parameters,
                     gpointer     data)
{
    if (g_strcmp0(signal_name, "update_operation") == 0)
    {
    }
}
`
void
get_update_operation_signal_from_agent (void)
{
	GVariant   *variant;
	GDBusProxy *proxy;
	GError     *error = NULL;
	gchar      *status = NULL;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                           G_DBUS_CALL_FLAGS_NONE,
                                           NULL,
                                           "kr.gooroom.agent",
                                           "/kr/gooroom/agent",
                                           "kr.gooroom.agent",
                                           NULL,
                                           &error);

	if (!proxy) goto done;

    g_signal_connect (proxy, "g-signal", G_CALLBACK (update_operation_cb), module);
     
done:
	if (error)
		g_error_free (error);
}
