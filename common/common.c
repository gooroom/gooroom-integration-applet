/*
 * Copyright (C) 2015-2019 Gooroom <gooroom@gooroom.kr>
 * Copyright (c) 2009 Brian Tarricone <brian@terricone.org>
 * Copyright (C) 1999 Olivier Fourdan <fourdan@xfce.org>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "common.h"
#include "panel-glib.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include <polkit/polkit.h>

#include <X11/Xatom.h>

#include <libsn/sn.h>


#define XFCE_SPAWN_STARTUP_TIMEOUT (30)

typedef struct
{
  /* startup notification data */
  SnLauncherContext *sn_launcher;
  guint              timeout_id;

  /* child watch data */
  guint              watch_id;
  GPid               pid;
  GClosure          *closure;
} XfceSpawnData;


/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_startup_timeout () */
static gboolean
xfce_spawn_startup_timeout (gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;
  GTimeVal       now;
  gdouble        elapsed;
  glong          tv_sec;
  glong          tv_usec;

  g_return_val_if_fail (spawn_data->sn_launcher != NULL, FALSE);

  /* determine the amount of elapsed time */
  g_get_current_time (&now);
  sn_launcher_context_get_last_active_time (spawn_data->sn_launcher, &tv_sec, &tv_usec);
  elapsed = now.tv_sec - tv_sec + ((gdouble) (now.tv_usec - tv_usec) / G_USEC_PER_SEC);

  return elapsed < XFCE_SPAWN_STARTUP_TIMEOUT;
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_startup_timeout_destroy () */
static void
xfce_spawn_startup_timeout_destroy (gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;
  GPid           pid;

  spawn_data->timeout_id = 0;

  if (G_LIKELY (spawn_data->sn_launcher != NULL))
   {
     /* abort the startup notification */
     sn_launcher_context_complete (spawn_data->sn_launcher);
     sn_launcher_context_unref (spawn_data->sn_launcher);
     spawn_data->sn_launcher = NULL;
   }

  /* if there is no closure to watch the child, also stop
   * the child watch */
  if (G_LIKELY (spawn_data->closure == NULL
                && spawn_data->watch_id != 0))
    {
      pid = spawn_data->pid;
      g_source_remove (spawn_data->watch_id);
      g_child_watch_add (pid,
                         (GChildWatchFunc) (void (*)(void)) g_spawn_close_pid,
                         NULL);
    }
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_startup_watch () */
static void
xfce_spawn_startup_watch (GPid     pid,
                          gint     status,
                          gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;
  GValue         instance_and_params[2] = { { 0, }, { 0, } };

  g_return_if_fail (spawn_data->pid == pid);

  if (G_UNLIKELY (spawn_data->closure != NULL))
    {
      /* xfce spawn has no instance */
      g_value_init (&instance_and_params[0], G_TYPE_POINTER);
      g_value_set_pointer (&instance_and_params[0], NULL);

      g_value_init (&instance_and_params[1], G_TYPE_INT);
      g_value_set_int (&instance_and_params[1], status);

      g_closure_set_marshal (spawn_data->closure, g_cclosure_marshal_VOID__INT);

      g_closure_invoke (spawn_data->closure, NULL,
                        2, instance_and_params, NULL);
    }

  /* don't leave zombies */
  g_spawn_close_pid (pid);
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_startup_watch_destroy () */
static void
xfce_spawn_startup_watch_destroy (gpointer user_data)
{
  XfceSpawnData *spawn_data = user_data;

  spawn_data->watch_id = 0;

  if (spawn_data->timeout_id != 0)
    g_source_remove (spawn_data->timeout_id);

  if (G_UNLIKELY (spawn_data->closure != NULL))
    {
      g_closure_invalidate (spawn_data->closure);
      g_closure_unref (spawn_data->closure);
    }

  g_slice_free (XfceSpawnData, spawn_data);
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_get_active_workspace_number () */
static gint
xfce_spawn_get_active_workspace_number (GdkScreen *screen)
{
  GdkWindow *root;
  gulong     bytes_after_ret = 0;
  gulong     nitems_ret = 0;
  guint     *prop_ret = NULL;
  Atom       _NET_CURRENT_DESKTOP;
  Atom       _WIN_WORKSPACE;
  Atom       type_ret = None;
  gint       format_ret;
  gint       ws_num = 0;

  gdk_error_trap_push ();

  root = gdk_screen_get_root_window (screen);

  /* determine the X atom values */
  _NET_CURRENT_DESKTOP = XInternAtom (GDK_WINDOW_XDISPLAY (root), "_NET_CURRENT_DESKTOP", False);
  _WIN_WORKSPACE = XInternAtom (GDK_WINDOW_XDISPLAY (root), "_WIN_WORKSPACE", False);

  if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root),
                          gdk_x11_get_default_root_xwindow(),
                          _NET_CURRENT_DESKTOP, 0, 32, False, XA_CARDINAL,
                          &type_ret, &format_ret, &nitems_ret, &bytes_after_ret,
                          (gpointer) &prop_ret) != Success)
    {
      if (XGetWindowProperty (GDK_WINDOW_XDISPLAY (root),
                              gdk_x11_get_default_root_xwindow(),
                              _WIN_WORKSPACE, 0, 32, False, XA_CARDINAL,
                              &type_ret, &format_ret, &nitems_ret, &bytes_after_ret,
                              (gpointer) &prop_ret) != Success)
        {
          if (G_UNLIKELY (prop_ret != NULL))
            {
              XFree (prop_ret);
              prop_ret = NULL;
            }
        }
    }

  if (G_LIKELY (prop_ret != NULL))
    {
      if (G_LIKELY (type_ret != None && format_ret != 0))
        ws_num = *prop_ret;
      XFree (prop_ret);
    }

  gdk_error_trap_pop_ignored ();

  return ws_num;
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_on_screen_with_child_watch () */
static gboolean
xfce_spawn_on_screen_with_child_watch (GdkScreen    *screen,
                                       const gchar  *working_directory,
                                       gchar       **argv,
                                       gchar       **envp,
                                       GSpawnFlags   flags,
                                       gboolean      startup_notify,
                                       guint32       startup_timestamp,
                                       const gchar  *startup_icon_name,
                                       GClosure     *child_watch_closure,
                                       GError      **error)
{
  gboolean            succeed;
  gchar             **cenvp;
  guint               n;
  guint               n_cenvp;
  gchar              *display_name;
  GPid                pid;
  XfceSpawnData      *spawn_data;
  SnLauncherContext  *sn_launcher = NULL;
  SnDisplay          *sn_display = NULL;
  gint                sn_workspace;
  const gchar        *startup_id;
  const gchar        *prgname;

  g_return_val_if_fail (screen == NULL || GDK_IS_SCREEN (screen), FALSE);
  g_return_val_if_fail ((flags & G_SPAWN_DO_NOT_REAP_CHILD) == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* setup the child environment (stripping $DESKTOP_STARTUP_ID and $DISPLAY) */
  if (G_LIKELY (envp == NULL))
	envp = g_get_environ ();

  for (n = 0; envp[n] != NULL; ++n);
    cenvp = g_new0 (gchar *, n + 3);
  for (n_cenvp = n = 0; envp[n] != NULL; ++n)
    {
      if (strncmp (envp[n], "DESKTOP_STARTUP_ID", 18) != 0
          && strncmp (envp[n], "DISPLAY", 7) != 0)
        cenvp[n_cenvp++] = g_strdup (envp[n]);
    }

  /* add the real display name for the screen */
  display_name = gdk_screen_make_display_name (screen);
  cenvp[n_cenvp++] = g_strconcat ("DISPLAY=", display_name, NULL);
  g_free (display_name);

  /* initialize the sn launcher context */
  if (G_LIKELY (startup_notify))
    {
      sn_display = sn_display_new (GDK_SCREEN_XDISPLAY (screen),
                                   (SnDisplayErrorTrapPush) (void (*)(void)) gdk_error_trap_push,
                                   (SnDisplayErrorTrapPop) (void (*)(void)) gdk_error_trap_pop);

      if (G_LIKELY (sn_display != NULL))
        {
          sn_launcher = sn_launcher_context_new (sn_display, GDK_SCREEN_XNUMBER (screen));
          if (G_LIKELY (sn_launcher != NULL))
            {
              /* initiate the sn launcher context */
              sn_workspace = xfce_spawn_get_active_workspace_number (screen);
              sn_launcher_context_set_workspace (sn_launcher, sn_workspace);
              sn_launcher_context_set_binary_name (sn_launcher, argv[0]);
              sn_launcher_context_set_icon_name (sn_launcher, startup_icon_name != NULL ?
                                                 startup_icon_name : "applications-other");

              if (G_LIKELY (!sn_launcher_context_get_initiated (sn_launcher)))
                {
                  prgname = g_get_prgname ();
                  sn_launcher_context_initiate (sn_launcher, prgname != NULL ?
                                                prgname : "unknown", argv[0],
                                                startup_timestamp);
                }

              /* add the real startup id to the child environment */
              startup_id = sn_launcher_context_get_startup_id (sn_launcher);
              if (G_LIKELY (startup_id != NULL))
                cenvp[n_cenvp++] = g_strconcat ("DESKTOP_STARTUP_ID=", startup_id, NULL);
            }
        }
    }

  /* watch the child process */
  flags |= G_SPAWN_DO_NOT_REAP_CHILD;

  /* test if the working directory exists */
  if (working_directory == NULL || *working_directory == '\0')
    {
      /* not worth a warning */
      working_directory = NULL;
    }
  else if (!g_file_test (working_directory, G_FILE_TEST_IS_DIR))
    {
      /* print warning for user */
      g_printerr (_("Working directory \"%s\" does not exist. It won't be used "
                    "when spawning \"%s\"."), working_directory, *argv);
      working_directory = NULL;
    }

  /* try to spawn the new process */
  succeed = g_spawn_async (working_directory, argv, cenvp, flags, NULL,
                           NULL, &pid, error);

  g_strfreev (cenvp);

  if (G_LIKELY (succeed))
    {
      /* setup data to watch the child */
      spawn_data = g_slice_new0 (XfceSpawnData);
      spawn_data->pid = pid;
      if (child_watch_closure != NULL)
        {
          spawn_data->closure = g_closure_ref (child_watch_closure);
          g_closure_sink (spawn_data->closure);
        }

      spawn_data->watch_id = g_child_watch_add_full (G_PRIORITY_LOW, pid,
                                                     xfce_spawn_startup_watch,
                                                     spawn_data,
                                                     xfce_spawn_startup_watch_destroy);

      if (G_LIKELY (sn_launcher != NULL))
        {
          /* start a timeout to stop the startup notification sequence after
           * a certain about of time, to handle applications that do not
           * properly implement startup notify */
          spawn_data->sn_launcher = sn_launcher;
          spawn_data->timeout_id = g_timeout_add_seconds_full (G_PRIORITY_LOW,
                                                               XFCE_SPAWN_STARTUP_TIMEOUT,
                                                               xfce_spawn_startup_timeout,
                                                               spawn_data,
                                                               xfce_spawn_startup_timeout_destroy);
        }
    }
  else
    {
      if (G_LIKELY (sn_launcher != NULL))
        {
          /* abort the startup notification sequence */
          sn_launcher_context_complete (sn_launcher);
          sn_launcher_context_unref (sn_launcher);
        }
    }

  /* release the sn display */
  if (G_LIKELY (sn_display != NULL))
    sn_display_unref (sn_display);

  return succeed;
}

/* Copied from libxfce4ui/tree/libxfce4ui/xfce-spawn.c:
 * xfce_spawn_on_screen () */
static gboolean
xfce_spawn_on_screen (GdkScreen    *screen,
                      const gchar  *working_directory,
                      gchar       **argv,
                      gchar       **envp,
                      GSpawnFlags   flags,
                      gboolean      startup_notify,
                      guint32       startup_timestamp,
                      const gchar  *startup_icon_name,
                      GError      **error)
{
  return xfce_spawn_on_screen_with_child_watch (screen, working_directory, argv,
                                                envp, flags, startup_notify,
                                                startup_timestamp, startup_icon_name,
                                                NULL, error);
}

static gchar *
desktop_working_directory_get (const gchar *id)
{
	gchar *wd = NULL;

	GDesktopAppInfo *dt_info = g_desktop_app_info_new (id);
	if (dt_info) {
		wd = g_desktop_app_info_get_string (dt_info, G_KEY_FILE_DESKTOP_KEY_PATH);
	}

	return wd;
}

static gboolean
launch_command (GdkScreen  *screen,
                const char *command,
                const char *workding_directory)
{
	gboolean    result = FALSE;
	GError     *error = NULL;
	char      **argv;
	int         argc;

	const gchar *s;
	GString *string = g_string_sized_new (256);

	for (s = command; *s; ++s) {
		if (*s == '%') {
			switch (*++s) {
				case '%':
					g_string_append_c (string, '%');
					break;
			}
		} else {
			g_string_append_c (string, *s);
		}
	}

	if (g_shell_parse_argv (string->str, &argc, &argv, NULL)) {
		result = xfce_spawn_on_screen (screen, workding_directory,
                                       argv, NULL, G_SPAWN_SEARCH_PATH,
                                       TRUE, gtk_get_current_event_time (),
                                       NULL, &error);
	}

	if (!result || error) {
		if (error) {
			g_warning ("Failed to launch application : %s", error->message);
			g_error_free (error);
			result = FALSE;
		}
	}

	g_string_free (string, TRUE);
	g_strfreev (argv);

	return result;
}

gboolean
launch_desktop_id (const char  *desktop_id, GdkScreen *screen)
{
	gboolean         retval;
	GDesktopAppInfo *appinfo = NULL;

	g_return_val_if_fail (desktop_id != NULL, FALSE);
	g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

	if (g_path_is_absolute (desktop_id)) {
		appinfo = g_desktop_app_info_new_from_filename (desktop_id);
	} else {
		gchar *full = g_strdup_printf ("/usr/share/applications/%s", desktop_id);
		if (full) { 
			appinfo = g_desktop_app_info_new_from_filename (full);
			g_free (full);
		}
	}

	if (appinfo == NULL)
		return FALSE;

	const char *cmdline = g_app_info_get_commandline (G_APP_INFO (appinfo));
	gchar *command = g_strdup (cmdline);
	command = g_strchug (command);
	if (!command || !command[0]) {
		g_free (command);
		retval = FALSE;
		goto out;
	}

	gchar *wd = desktop_working_directory_get (g_app_info_get_id (G_APP_INFO (appinfo)));
	retval = launch_command (screen, command, wd);
	g_free (wd);

	g_free (command);

out:
	g_object_unref (appinfo);

	return retval;
}

gboolean
is_local_user (void)
{
	gboolean ret = TRUE;

	struct passwd *user_entry = getpwnam (g_get_user_name ());
	if (user_entry) {
		gchar **tokens = g_strsplit (user_entry->pw_gecos, ",", -1);

		if (g_strv_length (tokens) > 4 ) {
			if (tokens[4] && (g_str_equal (tokens[4], "gooroom-account") ||
                              g_str_equal (tokens[4], "google-account") ||
                              g_str_equal (tokens[4], "naver-account"))) {
				ret = FALSE;
			}
		}

		g_strfreev (tokens);
	}

	return ret;
}

gboolean
is_admin_group (void)
{
	gchar *cmd;
	gchar *program;
	gchar *output;
	gboolean ret = FALSE;

	program = g_find_program_in_path ("groups");
	cmd = g_strdup_printf ("%s", program);

	if (g_spawn_command_line_sync (cmd, &output, NULL, NULL, NULL)) {
		if (output) {
			guint i = 0;
			gchar **lines = g_strsplit (output, "\n", -1);
			for (i = 0; lines[i] != NULL; i++) {
				guint j = 0;
				gchar **groups = g_strsplit (lines[i], " ", -1);
				for (j = 0; groups[j] != NULL; j++) {
					if (g_strcmp0 (groups[j], "sudo") == 0) {
						ret = TRUE;
						break;
					}
				}
				g_strfreev (groups);
			}
			g_strfreev (lines);

			g_free (output);
		}
	}

	g_free (cmd);
	g_free (program);

	return ret;
}

gboolean
authenticate (const gchar *action_id)
{
	GPermission *permission;
	permission = polkit_permission_new_sync (action_id, NULL, NULL, NULL);

	if (!g_permission_get_allowed (permission)) {
		if (g_permission_acquire (permission, NULL, NULL)) {
			return TRUE;
		}
		return FALSE;
	}

	return TRUE;
}

static gboolean
get_object_path (gchar **object_path, const gchar *service_name)
{
	GVariant   *variant;
	GDBusProxy *proxy;
	GError     *error = NULL;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
			G_DBUS_CALL_FLAGS_NONE, NULL,
			"org.freedesktop.systemd1",
			"/org/freedesktop/systemd1",
			"org.freedesktop.systemd1.Manager",
			NULL, &error);

	if (!proxy) {
		g_error_free (error);
		return FALSE;
	}

	variant = g_dbus_proxy_call_sync (proxy, "GetUnit",
			g_variant_new ("(s)", service_name),
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if (!variant) {
		g_error_free (error);
	} else {
		g_variant_get (variant, "(o)", object_path);
		g_variant_unref (variant);
	}

	g_object_unref (proxy);

	return TRUE;
}

gboolean
is_systemd_service_available (const gchar *service_name)
{
	gboolean    ret = TRUE;
	GVariant   *variant;
	GDBusProxy *proxy;
	GError     *error = NULL;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
			G_DBUS_CALL_FLAGS_NONE, NULL,
			"org.freedesktop.systemd1",
			"/org/freedesktop/systemd1",
			"org.freedesktop.systemd1.Manager",
			NULL, &error);

	if (!proxy) {
		g_error_free (error);
		return FALSE;
	}

	variant = g_dbus_proxy_call_sync (proxy, "GetUnitFileState",
			g_variant_new ("(s)", service_name),
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if (!variant) {
		g_error_free (error);
		ret = FALSE;
	}

	g_object_unref (proxy);

	return ret;
}

gboolean
is_standalone_mode (void)
{
	gboolean ret = FALSE;
	GError *error = NULL;
	GKeyFile *keyfile = NULL;
	gchar *client_name = NULL;

	keyfile = g_key_file_new ();
	g_key_file_load_from_file (keyfile, GOOROOM_MANAGEMENT_SERVER_CONF, G_KEY_FILE_KEEP_COMMENTS, &error);

	if (error == NULL) {
		if (g_key_file_has_group (keyfile, "certificate")) {
			client_name = g_key_file_get_string (keyfile, "certificate", "client_name", NULL);
		}
	} else {
		g_clear_error (&error);
	}

	ret = (client_name == NULL) ? TRUE : FALSE;

	g_key_file_free (keyfile);
	g_free (client_name);

	return ret;
}

gboolean
is_systemd_service_active (const gchar *service_name)
{
	gboolean ret = FALSE;

	GVariant   *variant;
	GDBusProxy *proxy;
	GError     *error = NULL;
	gchar      *obj_path = NULL;

	get_object_path (&obj_path, service_name);
	if (!obj_path) {
		goto done;
	}

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
			G_DBUS_CALL_FLAGS_NONE, NULL,
			"org.freedesktop.systemd1",
			obj_path,
			"org.freedesktop.DBus.Properties",
			NULL, &error);

	if (!proxy) {
		goto done;
	}

	variant = g_dbus_proxy_call_sync (proxy, "GetAll",
			g_variant_new ("(s)", "org.freedesktop.systemd1.Unit"),
			G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

	if (variant) {
		gchar *output = NULL;
		GVariant *asv = g_variant_get_child_value(variant, 0);
		GVariant *value = g_variant_lookup_value(asv, "ActiveState", NULL);
		if(value && g_variant_is_of_type(value, G_VARIANT_TYPE_STRING)) {
			output = g_variant_dup_string(value, NULL);
			if (g_strcmp0 (output, "active") == 0) {
				ret = TRUE;;
			}
			g_free (output);
		}

		g_variant_unref (variant);
	}

	g_object_unref (proxy);

done:
	if (error)
		g_error_free (error);

	return ret;
}
