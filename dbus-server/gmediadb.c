/*
 *      gmediadb.c
 *      
 *      Copyright 2009 Brett Mravec <brett.mravec@gmail.com>
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
#include "gmediadb-glue.h"
#include "media-object.h"

G_DEFINE_TYPE(GMediaDB, gmediadb, G_TYPE_OBJECT)

struct _GMediaDBPrivate {
    GMainLoop *loop;
    guint ref_cnt;
    
    DBusGConnection *conn;
    DBusGProxy *proxy;
    
    GHashTable *mos;
};

static guint signal_add;
static guint signal_remove;

static void
gmediadb_finalize (GObject *object)
{
    GMediaDB *self = GMEDIADB (object);
    
    G_OBJECT_CLASS (gmediadb_parent_class)->finalize (object);
}

static void
gmediadb_class_init (GMediaDBClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);
    
    g_type_class_add_private ((gpointer) klass, sizeof (GMediaDBPrivate));
    
    object_class->finalize = gmediadb_finalize;
    
    signal_add = g_signal_new ("add-entry", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);
    
    signal_remove = g_signal_new ("remove-entry", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);
    
    dbus_g_object_type_install_info (GMEDIADB_TYPE,
                                     &dbus_glib_gmediadb_object_info);
}

static void
gmediadb_init (GMediaDB *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), GMEDIADB_TYPE, GMediaDBPrivate);
    
    self->priv->ref_cnt = 0;
    self->priv->loop = g_main_loop_new (NULL, FALSE);
    
    self->priv->conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
    self->priv->proxy = dbus_g_proxy_new_for_name (self->priv->conn,
        DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);
    
    org_freedesktop_DBus_request_name (self->priv->proxy,
        GMEDIADB_DBUS_SERVICE, DBUS_NAME_FLAG_DO_NOT_QUEUE, NULL, NULL);
    
    self->priv->mos = g_hash_table_new_full (g_str_hash,
        g_str_equal, g_free, g_object_unref);
    
    dbus_g_connection_register_g_object (self->priv->conn,
        GMEDIADB_DBUS_PATH, G_OBJECT (self));
}

GMediaDB*
gmediadb_new ()
{
    return g_object_new (GMEDIADB_TYPE, NULL);
}

void
gmediadb_run (GMediaDB *self)
{
   g_main_loop_run (self->priv->loop);
}

void
gmediadb_stop (GMediaDB *self)
{
   g_main_loop_quit (self->priv->loop);
}

void
gmediadb_ref (GMediaDB *self)
{
    self->priv->ref_cnt++;
}

void
gmediadb_unref (GMediaDB *self)
{
    self->priv->ref_cnt--;
    if (self->priv->ref_cnt <= 0) {
        gmediadb_stop (self);
    }
}

gboolean
gmediadb_register_type (GMediaDB *self, gchar *name, GError **error)
{
    MediaObject *mo = g_hash_table_lookup (self->priv->mos, name);
    
    if (!mo) {
        mo = media_object_new (self->priv->conn, name, self);
        g_object_ref (mo);
        g_hash_table_insert (self->priv->mos, g_strdup (name), mo);
    }
}

int
main (int argc, char *argv[])
{
    GMediaDB *gdb;
    
    g_type_init ();
    if (!g_thread_supported ())
        g_thread_init (NULL);
    
    dbus_g_thread_init ();
    
    gdb = gmediadb_new ();
    
    gmediadb_run (gdb);
}

