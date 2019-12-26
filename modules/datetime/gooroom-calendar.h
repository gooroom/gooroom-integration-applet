/*
 *  Copyright (C) 2015-2019 Gooroom <gooroom@gooroom.kr>
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


#ifndef __GOOROOM_CALENDAR_H__
#define __GOOROOM_CALENDAR_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GOOROOM_TYPE_CALENDAR            (gooroom_calendar_get_type ())
#define GOOROOM_CALENDAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOOROOM_TYPE_CALENDAR, GooroomCalendar))
#define GOOROOM_CALENDAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GOOROOM_TYPE_CALENDAR, GooroomCalendarClass))
#define GOOROOM_IS_CALENDAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOOROOM_TYPE_CALENDAR))
#define GOOROOM_IS_CALENDAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOOROOM_TYPE_CALENDAR))
#define GOOROOM_CALENDAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GOOROOM_TYPE_CALENDAR, GooroomCalendarClass))

//typedef struct _GooroomCalendarPrivate GooroomCalendarPrivate;
typedef struct _GooroomCalendarClass   GooroomCalendarClass;
typedef struct _GooroomCalendar        GooroomCalendar;

struct _GooroomCalendarClass
{
	GtkCalendarClass __parent_class__;
};

struct _GooroomCalendar
{
	GtkCalendar __parent__;
};


GType      gooroom_calendar_get_type   (void) G_GNUC_CONST;

GtkWidget *gooroom_calendar_new        (void);


G_END_DECLS

#endif /* !__GOOROOM_CALENDAR_H__ */
