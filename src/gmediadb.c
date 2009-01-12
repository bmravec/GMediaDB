/*
 *      gmediadb.c
 *      
 *      Copyright 2009 Brett Mravec <bmravec@purdue.edu>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include <glib.h>
#include <glib/gthread.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>

#include "gmediadb.h"
#include "media-object.h"

typedef struct _GMediaDBPrivate GMediaDBPrivate;

#define GMEDIA_DB_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GMEDIA_DB_TYPE, GMediaDBPrivate))

struct _GMediaDBPrivate
{
	GMainLoop *loop;
	gint ref_cnt;
	MediaObject *music, *tv_shows, *videos, *music_videos, *pictures;
};

static void gmedia_db_class_init (GMediaDBClass *klass);
static void gmedia_db_init (GMediaDB *self);

/* Local data */
static GObjectClass *parent_class = NULL;

GType
gmedia_db_get_type (void)
{
	static GType self_type = 0;
	if (!self_type) {
		static const GTypeInfo self_info = 
		{
			sizeof (GMediaDBClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc) gmedia_db_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof (GMediaDB),
			0,
			(GInstanceInitFunc) gmedia_db_init,
			NULL /* value_table */
		};
		
		self_type = g_type_register_static (G_TYPE_OBJECT, "GMediaDB", &self_info, 0);
	}
	
	return self_type;
}

static void
gmedia_db_class_init (GMediaDBClass *klass)
{
	parent_class = (GObjectClass*) g_type_class_peek (G_TYPE_OBJECT);
	g_type_class_add_private ((gpointer) klass, sizeof (GMediaDBPrivate));
}

static void
gmedia_db_init (GMediaDB *self)
{
	GMediaDBPrivate *priv = GMEDIA_DB_GET_PRIVATE (self);
	DBusGConnection *conn;
	DBusGProxy *proxy;
	GError *error = NULL;
	guint result;
	
	priv->loop = g_main_loop_new (NULL, FALSE);
	
	priv->ref_cnt = 0;

	conn = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	proxy = dbus_g_proxy_new_for_name (conn, DBUS_SERVICE_DBUS,
		DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
	
	org_freedesktop_DBus_request_name (proxy, GMEDIADB_DBUS_SERVICE,
		DBUS_NAME_FLAG_DO_NOT_QUEUE, &result, &error);
		
	priv->music = media_object_new (conn, "Music", self);
	priv->tv_shows = media_object_new (conn, "TVShows", self);
	priv->videos = media_object_new (conn, "Videos", self);
	priv->music_videos = media_object_new (conn, "MusicVideos", self);
	priv->pictures = media_object_new (conn, "Pictures", self);
}

GObject*
gmedia_db_new (void)
{
	return (GObject*) g_object_new (GMEDIA_DB_TYPE, NULL);
}

void
gmedia_db_run (GMediaDB *self)
{
	g_main_loop_run (GMEDIA_DB_GET_PRIVATE (self)->loop);
}

void
gmedia_db_stop (GMediaDB *self)
{
	g_main_loop_quit (GMEDIA_DB_GET_PRIVATE (self)->loop);
}

void
gmedia_db_ref (GMediaDB *self)
{
	GMEDIA_DB_GET_PRIVATE (self)->ref_cnt++;
}

void
gmedia_db_unref (GMediaDB *self)
{
	GMediaDBPrivate *priv = GMEDIA_DB_GET_PRIVATE (self);
	
	priv->ref_cnt--;
	if (priv->ref_cnt <= 0) {
		gmedia_db_stop (self);
	}
}

int
main (int argc, char **argv)
{
	GMediaDB *gdb;
	
	g_type_init ();

	if (!g_thread_supported ())
		g_thread_init (NULL);

	dbus_g_thread_init ();

	gdb = (GMediaDB*) gmedia_db_new ();

	gmedia_db_run (gdb);
}

