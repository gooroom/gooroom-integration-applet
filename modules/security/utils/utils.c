/*
 * Copyright (C) 2015-2019 Gooroom <gooroom@gooroom.kr>
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

#include <json-c/json.h>


json_object *
JSON_OBJECT_GET (json_object *obj, const gchar *key)
{
	if (!obj) return NULL;

	json_object *ret_obj = NULL;

	json_object_object_get_ex (obj, key, &ret_obj);

	return ret_obj;
}

void
send_taking_measure_signal_to_self (void)
{
	gchar *pkexec, *cmdline;

	pkexec = g_find_program_in_path ("pkexec");
	cmdline = g_strdup_printf ("%s %s", pkexec, GOOROOM_LOGPARSER_SEEKTIME_HELPER);

	g_spawn_command_line_sync (cmdline, NULL, NULL, NULL, NULL);

	g_free (pkexec);
	g_free (cmdline);
}

void
send_taking_measures_signal_to_agent (void)
{
	GVariant   *variant;
	GDBusProxy *proxy;
	GError     *error = NULL;
	gchar      *status = NULL;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
			G_DBUS_CALL_FLAGS_NONE,
			NULL,
			"kr.gooroom.agent",
			"/kr/gooroom/agent",
			"kr.gooroom.agent",
			NULL,
			&error);

	if (!proxy) goto done;

	const gchar *arg = "{\"module\":{\"module_name\":\"log\",\"task\":{\"task_name\":\"clear_security_alarm\",\"in\":{}}}}";

	variant = g_dbus_proxy_call_sync (proxy, "do_task",
				g_variant_new ("(s)", arg),
				G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if (variant) g_variant_unref (variant);

	g_object_unref (proxy);

done:
	if (error)
		g_error_free (error);
}

static void
run_security_log_parser_async_done (GPid child_pid, gint status, gpointer user_data)
{
	g_spawn_close_pid (child_pid);
}

gboolean
run_security_log_parser_async (GPid *pid, GIOFunc callback_func, gpointer data)
{
	GPid child_pid;
	gboolean ret = FALSE;
	gint stdout_fd;
	const gchar *lang;
	gchar *seektime = NULL, *pkexec = NULL, *cmdline = NULL;

	lang = g_getenv ("LANG");
	pkexec = g_find_program_in_path ("pkexec");

	g_file_get_contents (GOOROOM_SECURITY_LOGPARSER_NEXT_SEEKTIME, &seektime, NULL, NULL);

	if (seektime) {
		cmdline = g_strdup_printf ("%s %s %s %s", pkexec,
                                   GOOROOM_SECURITY_LOGPARSER_WRAPPER, seektime, lang);
	} else {
		cmdline = g_strdup_printf ("%s %s %s", pkexec,
                                   GOOROOM_SECURITY_LOGPARSER_WRAPPER, lang);
	}

	gchar **arr_cmd = g_strsplit (cmdline, " ", -1);

	if (g_spawn_async_with_pipes (NULL,
                                  arr_cmd,
                                  NULL,
                                  G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                  NULL,
                                  NULL,
                                  &child_pid,
                                  NULL,
                                  &stdout_fd,
                                  NULL,
                                  NULL))
	{
		if (pid) *pid = child_pid;

		g_child_watch_add (child_pid, (GChildWatchFunc)run_security_log_parser_async_done, NULL);

		GIOChannel *io_channel = g_io_channel_unix_new (stdout_fd);
		g_io_channel_set_flags (io_channel, G_IO_FLAG_NONBLOCK, NULL);
		g_io_channel_set_encoding (io_channel, NULL, NULL);
		g_io_channel_set_buffered (io_channel, FALSE);
		g_io_channel_set_close_on_unref (io_channel, TRUE);
		g_io_add_watch (io_channel, G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP, callback_func, data);
		g_io_channel_unref (io_channel);
		ret = TRUE;
	}

	g_strfreev (arr_cmd);

	g_free (pkexec);
	g_free (cmdline);

	return ret;
}
