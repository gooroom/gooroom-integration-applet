/*
 *  Copyright (C) 2015-2019 Gooroom <gooroom@gooroom.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __GOOROOM_INTEGRATION_APPLET_H__
#define __GOOROOM_INTEGRATION_APPLET_H__

G_BEGIN_DECLS

#include <libgnome-panel/gp-applet.h>

#define GOOROOM_TYPE_INTEGRATION_APPLET           (gooroom_integration_applet_get_type ())
#define GOOROOM_INTEGRATION_APPLET(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOOROOM_TYPE_INTEGRATION_APPLET, GooroomIntegrationApplet))
#define GOOROOM_INTEGRATION_APPLET_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST    ((obj), GOOROOM_TYPE_INTEGRATION_APPLET, GooroomIntegrationAppletClass))
#define GOOROOM_IS_INTEGRATION_APPLET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOOROOM_TYPE_INTEGRATION_APPLET))
#define GOOROOM_IS_INTEGRATION_APPLET_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE    ((obj), GOOROOM_TYPE_INTEGRATION_APPLET))
#define GOOROOM_INTEGRATION_APPLET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS  ((obj), GOOROOM_TYPE_INTEGRATION_APPLET, GooroomIntegrationAppletClass))

typedef struct _GooroomIntegrationApplet        GooroomIntegrationApplet;
typedef struct _GooroomIntegrationAppletClass   GooroomIntegrationAppletClass;
typedef struct _GooroomIntegrationAppletPrivate GooroomIntegrationAppletPrivate;

struct _GooroomIntegrationApplet {
	GpApplet                         parent;
	GooroomIntegrationAppletPrivate *priv;
};

struct _GooroomIntegrationAppletClass {
	GpAppletClass                    parent_class;
};

GType gooroom_integration_applet_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GOOROOM_INTEGRATION_APPLET_H__*/
