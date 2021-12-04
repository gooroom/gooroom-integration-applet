/*
 *  Copyright (C) 2015-2019 Hodong Kim <cogniti@gmail.com>
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
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include <nimf/nimf.h>

#define NIMF_TYPE_GOOROOM            (nimf_gooroom_get_type ())
#define NIMF_GOOROOM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NIMF_TYPE_GOOROOM, NimfGooroom))
#define NIMF_GOOROOM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NIMF_TYPE_GOOROOM, NimfGooroomClass))
#define NIMF_IS_GOOROOM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NIMF_TYPE_GOOROOM))
#define NIMF_IS_GOOROOM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), NIMF_TYPE_GOOROOM))
#define NIMF_GOOROOM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NIMF_TYPE_GOOROOM, NimfGooroomClass))


typedef struct _NimfGooroom NimfGooroom;
typedef struct _NimfGooroomClass NimfGooroomClass;


struct _NimfGooroomClass
{
	NimfServiceClass parent_class;
};

struct _NimfGooroom
{
	NimfService parent;

	gchar    *id;
	gchar    *engine_id;
	gchar    *icon_name;

	gboolean  active;

	guint     reg_id;
	guint     owner_id;
	guint     timeout_id;

	GDBusConnection *connection;
};

GType nimf_gooroom_get_type (void) G_GNUC_CONST;

G_DEFINE_DYNAMIC_TYPE (NimfGooroom, nimf_gooroom, NIMF_TYPE_SERVICE)


static void
on_nimf_engine_changed_cb (NimfServer  *server,
                           const gchar *engine_id,
                           const gchar *icon_name,
                           gpointer     data)
{
	NimfGooroom *ng = NIMF_GOOROOM (data);

	g_free (ng->engine_id);
	ng->engine_id = (engine_id) ? g_strdup (engine_id) : g_strdup ("");

	g_free (ng->icon_name);
	ng->icon_name = g_strdup (icon_name);

	if (ng->connection) {
		g_dbus_connection_emit_signal (ng->connection,
                                       NULL,
                                       "/kr/gooroom/nimf/Service",
                                       "kr.gooroom.nimf.Service",
                                       "EngineChanged",
                                       g_variant_new ("(ss)", ng->engine_id, ng->icon_name),
                                       NULL);
	}
}

static void
on_nimf_engine_status_changed_cb (NimfServer  *server,
                                  const gchar *engine_id,
                                  const gchar *icon_name,
                                  gpointer     data)
{
	NimfGooroom *ng = NIMF_GOOROOM (data);

    if (!ng->connection)
        return;

	if (g_str_equal (ng->engine_id, engine_id)) {
		g_dbus_connection_emit_signal (ng->connection,
                                       NULL,
                                       "/kr/gooroom/nimf/Service",
                                       "kr.gooroom.nimf.Service",
                                       "EngineStatusChanged",
                                       g_variant_new ("(ss)", engine_id, icon_name),
                                       NULL);
	}
}

static void
handle_method_call (GDBusConnection *conn,
                    const gchar *sender,
                    const gchar *object_path,
                    const gchar *interface_name,
                    const gchar *method_name,
                    GVariant *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer data)
{
	NimfGooroom *ng = NIMF_GOOROOM (data);

	if (!g_strcmp0 (method_name, "ChangeEngine")) {
		const gchar *engine_id, *method_id;
		g_variant_get (parameters, "(&s&s)", &engine_id, &method_id);

		if (!engine_id || !method_id) {
			g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", FALSE));
			return;
		}

		NimfServer *server = nimf_server_get_default ();

		if (!g_strcmp0 (method_id, "")) {
			nimf_server_change_engine_by_id (server, engine_id);
		} else {
			nimf_server_change_engine (server, engine_id, method_id);
		}
        g_dbus_method_invocation_return_value (invocation, g_variant_new ("(b)", TRUE));
	} else if (!g_strcmp0 (method_name, "GetStatus")) {
		g_dbus_method_invocation_return_value (invocation,
				g_variant_new ("(ss)", ng->engine_id, ng->icon_name));
	} else {
		g_dbus_method_invocation_return_error (invocation, G_DBUS_ERROR, G_DBUS_ERROR_FAILED, "No such method: %s", method_name);
	}
}

static const gchar introspection_xml[] =
	"<node>"
	"  <interface name='kr.gooroom.nimf.Service'>"
	"    <method name='ChangeEngine'>"
	"      <arg type='s' name='engine_id' direction='in'/>"
	"      <arg type='s' name='method_id' direction='in'/>"
	"      <arg type='b' direction='out'/>"
	"    </method>"
	"    <method name='GetStatus'>"
	"      <arg type='s' direction='out'/>"
	"      <arg type='s' direction='out'/>"
	"    </method>"
    "    <signal name='EngineChanged'>"
    "      <arg name='engine_id' type='s'/>"
    "      <arg name='icon_name' type='s'/>"
    "    </signal>"
    "    <signal name='EngineStatusChanged'>"
    "      <arg name='engine_id' type='s'/>"
    "      <arg name='icon_name' type='s'/>"
    "    </signal>"
    "    <signal name='Stopped'/>"
	"  </interface>"
	"</node>";

static const GDBusInterfaceVTable interface_vtable = {
	handle_method_call,
	NULL,
	NULL
};

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer        *data)
{
	GDBusNodeInfo *introspection_data;
	NimfGooroom *ng = NIMF_GOOROOM (data);

	ng->connection = connection;

	// register object
	introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);

	ng->reg_id = g_dbus_connection_register_object (ng->connection,
                                                    "/kr/gooroom/nimf/Service",
                                                    introspection_data->interfaces[0],
                                                    &interface_vtable,
                                                    ng, NULL,
                                                    NULL);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer        *data)
{
}

static void
nimf_gooroom_dbus_init (NimfGooroom *ng)
{
	NimfServer *server = nimf_server_get_default ();

	if (ng->timeout_id) {
		g_source_remove (ng->timeout_id);
		ng->timeout_id = 0;
	}

	ng->owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                   "kr.gooroom.nimf.Service",
                                   G_BUS_NAME_OWNER_FLAGS_NONE,
                                   (GBusAcquiredCallback) on_bus_acquired,
                                   NULL,
                                   (GBusNameLostCallback) on_name_lost,
                                   ng, NULL);

	g_signal_connect (server, "engine-changed", G_CALLBACK (on_nimf_engine_changed_cb), ng);
	g_signal_connect (server, "engine-status-changed",
                      G_CALLBACK (on_nimf_engine_status_changed_cb), ng);
}

static gboolean
on_timeout_cb (gpointer data)
{
	nimf_gooroom_dbus_init (NIMF_GOOROOM (data));

	return FALSE;
}

static gboolean
nimf_gooroom_start (NimfService *service)
{
	NimfGooroom *ng = NIMF_GOOROOM (service);

	if (ng->active)
		return TRUE;

	ng->timeout_id = g_timeout_add (3000, (GSourceFunc) on_timeout_cb, ng);

	return ng->active = TRUE;
}

static void
nimf_gooroom_stop (NimfService *service)
{
	NimfGooroom *ng = NIMF_GOOROOM (service);

	if (!ng->active)
		return;

	g_dbus_connection_emit_signal (ng->connection,
                                   NULL,
                                   "/kr/gooroom/nimf/Service",
                                   "kr.gooroom.nimf.Service",
                                   "Stopped",
                                   NULL,
                                   NULL);

	if (ng->owner_id) {
		g_bus_unown_name (ng->owner_id);
		ng->owner_id = 0;
	}

	if (ng->timeout_id) {
		g_source_remove (ng->timeout_id);
		ng->timeout_id = 0;
	}

	g_signal_handlers_disconnect_by_data (nimf_server_get_default (), ng);

	if (ng->reg_id) {
		g_dbus_connection_unregister_object (ng->connection, ng->reg_id);
		ng->reg_id = 0;
	}

	ng->active = FALSE;
}

const gchar *
nimf_gooroom_get_id (NimfService *service)
{
  return NIMF_GOOROOM (service)->id;
}

static gboolean
nimf_gooroom_is_active (NimfService *service)
{
	return NIMF_GOOROOM (service)->active;
}

static void
nimf_gooroom_finalize (GObject *object)
{
	NimfGooroom *ng = NIMF_GOOROOM (object);

	if (ng->active)
		nimf_gooroom_stop (NIMF_SERVICE (ng));

	g_free (ng->id);
	g_free (ng->engine_id);
	g_free (ng->icon_name);

	G_OBJECT_CLASS (nimf_gooroom_parent_class)->finalize (object);
}

static void
nimf_gooroom_init (NimfGooroom *ng)
{
	ng->reg_id     = 0;
	ng->owner_id   = 0;
	ng->timeout_id = 0;
	ng->active     = FALSE;
	ng->engine_id  = g_strdup ("");
	ng->icon_name  = g_strdup ("nimf-focus-out");
	ng->id         = g_strdup ("nimf-gooroom");
}

static void
nimf_gooroom_class_init (NimfGooroomClass *class)
{
	GObjectClass *object_class;
	NimfServiceClass *service_class;

	object_class = G_OBJECT_CLASS (class);
	service_class = NIMF_SERVICE_CLASS (class);

	object_class->finalize = nimf_gooroom_finalize;

	service_class->stop      = nimf_gooroom_stop;
	service_class->start     = nimf_gooroom_start;
	service_class->get_id    = nimf_gooroom_get_id;
	service_class->is_active = nimf_gooroom_is_active;
}

static void
nimf_gooroom_class_finalize (NimfGooroomClass *class)
{
}

void module_register_type (GTypeModule *type_module)
{
  nimf_gooroom_register_type (type_module);
}

GType module_get_type ()
{
  return nimf_gooroom_get_type ();
}
