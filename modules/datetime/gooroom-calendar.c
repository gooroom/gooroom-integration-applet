/*
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
 */

#include <glib.h>
#include <gtk/gtk.h>

#include "gooroom-calendar.h"


G_DEFINE_TYPE (GooroomCalendar, gooroom_calendar, GTK_TYPE_CALENDAR);



static gboolean
gooroom_calendar_motion_notify (GtkWidget      *widget,
                                GdkEventMotion *event)
{
	return TRUE;
}

static gboolean
gooroom_calendar_scroll (GtkWidget      *widget,
                         GdkEventScroll *event)
{
	return FALSE;
}

static void
gooroom_calendar_init (GooroomCalendar *calendar)
{
	GDateTime *dt;
	GtkStyleContext *context;

	dt = g_date_time_new_now_local ();

	gtk_calendar_select_month (GTK_CALENDAR (calendar),
                               g_date_time_get_month (dt) - 1,
                               g_date_time_get_year (dt));

	gtk_calendar_select_day (GTK_CALENDAR (calendar),
                             g_date_time_get_day_of_month (dt));

	g_date_time_unref (dt);

    context = gtk_widget_get_style_context (GTK_WIDGET (calendar));
	gtk_style_context_add_class (context, "applet-calendar");
}

static void
gooroom_calendar_class_init (GooroomCalendarClass *klass)
{
	GtkWidgetClass *widget_class;

	widget_class = GTK_WIDGET_CLASS (klass);

	widget_class->scroll_event = gooroom_calendar_scroll;
	widget_class->motion_notify_event = gooroom_calendar_motion_notify;
}

GtkWidget *
gooroom_calendar_new (void)
{
	GtkWidget *calendar;

	calendar = g_object_new (GOOROOM_TYPE_CALENDAR, NULL);

	return calendar;
}
