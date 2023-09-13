/*
 * Copyright (C) 2023 Gooroom <gooroom@gooroom.kr>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <glib.h>
#include <gio/gio.h>
#include <glib/gi18n.h>


#define TABLET_MODE_PATH    "/etc/gooroom"
#define TABLET_MODE_FILE    ".tablet-mode" 

static gboolean opt_delete = FALSE;

static GOptionEntry options[] =
{
	{ "delete", 'd', 0, G_OPTION_ARG_NONE,   &opt_delete,    NULL, NULL },
	{ NULL }
};

int
main (int argc, char **argv)
{
	gchar *cmd = NULL;
	GOptionContext *context;

	/* Initialize i18n */
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	if (opt_delete) {
		gchar *rm = g_find_program_in_path ("rm");
		cmd = g_strdup_printf ("%s -rf %s/%s", rm, TABLET_MODE_PATH, TABLET_MODE_FILE);
	} else {
		if (!g_file_test (TABLET_MODE_PATH, G_FILE_TEST_EXISTS))
			g_mkdir_with_parents (TABLET_MODE_PATH, 0755);

		gchar *touch = g_find_program_in_path ("touch");
		cmd = g_strdup_printf ("%s %s/%s", touch, TABLET_MODE_PATH, TABLET_MODE_FILE);
	}

	g_spawn_command_line_sync (cmd, NULL, NULL, NULL, NULL);

	g_clear_pointer (&cmd, g_free);

	return EXIT_FAILURE;
}
