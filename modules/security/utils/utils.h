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


#ifndef _UTILS_H_
#define _UTILS_H_


#include <glib.h>
#include <gio/gio.h>

#include <json-c/json.h>

#define GOOROOM_SECURITY_LOGPARSER_SEEKTIME      "/var/tmp/GOOROOM-SECURITY-LOGPARSER-SEEKTIME"
#define GOOROOM_SECURITY_STATUS_VULNERABLE       "/var/tmp/GOOROOM-SECURITY-STATUS-VULNERABLE"
#define GOOROOM_SECURITY_LOGPARSER_NEXT_SEEKTIME "/var/tmp/GOOROOM-SECURITY-LOGPARSER-NEXT-SEEKTIME"

G_BEGIN_DECLS


json_object *JSON_OBJECT_GET                       (json_object *obj,
                                                    const gchar *key);

gboolean     run_security_log_parser_async         (GPid     *pid,
                                                    GIOFunc   callback_func,
                                                    gpointer  data);

void         send_taking_measures_signal_to_agent  (void);
void         send_taking_measure_signal_to_self    (void);


G_END_DECLS


#endif /* _UTILS_H */
