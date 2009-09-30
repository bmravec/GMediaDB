/*
 *      gmediadb.c
 *
 *      Copyright 2009 Brett Mravec <brett.mravec@gmail.com>
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include <stdio.h>
#include "gmediadb.h"

#include <dbus/dbus-glib.h>

G_DEFINE_TYPE(GMediaDB, gmediadb, G_TYPE_OBJECT)

struct _GMediaDBPrivate {
    DBusGProxy *db_proxy;
    DBusGProxy *mo_proxy;
    DBusGConnection *conn;

    gchar *mtype;
};

static guint signal_add;
static guint signal_update;
static guint signal_remove;

void
media_added_cb (DBusGProxy *proxy, guint id, gpointer user_data)
{
    g_signal_emit (G_OBJECT (user_data), signal_add, 0, id);
}

void
media_updated_cb (DBusGProxy *proxy, guint id, gpointer user_data)
{
    g_signal_emit (G_OBJECT (user_data), signal_update, 0, id);
}

void
media_removed_cb (DBusGProxy *proxy, guint id, gpointer user_data)
{
    g_signal_emit (G_OBJECT (user_data), signal_remove, 0, id);
}

static void
gmediadb_finalize (GObject *object)
{
    GMediaDB *self = GMEDIADB (object);

    //TODO: Write data to disk and free associated memory

    if (self->priv->mo_proxy) {
        dbus_g_proxy_call (self->priv->mo_proxy, "unref", NULL,
            G_TYPE_INVALID, G_TYPE_INVALID);

        self->priv->mo_proxy = NULL;
    }

    g_free (self->priv->mtype);

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

    signal_update = g_signal_new ("update-entry", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);

    signal_remove = g_signal_new ("remove-entry", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
gmediadb_init (GMediaDB *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), GMEDIADB_TYPE, GMediaDBPrivate);

    self->priv->conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
    if (!self->priv->conn) {
        g_printerr ("Failed to open connection to dbus\n");
        return;
    }

    self->priv->db_proxy = dbus_g_proxy_new_for_name (self->priv->conn,
        "org.gnome.GMediaDB", "/org/gnome/GMediaDB", "org.gnome.GMediaDB");
}

GMediaDB*
gmediadb_new (const gchar *mediatype)
{
    GMediaDB *self = g_object_new (GMEDIADB_TYPE, NULL);

    self->priv->mtype = g_strdup (mediatype);

    gchar *new_path = g_strdup_printf ("/org/gnome/GMediaDB/%s", mediatype);

    if (!dbus_g_proxy_call (self->priv->db_proxy, "register_type", NULL,
        G_TYPE_STRING, mediatype, G_TYPE_INVALID, G_TYPE_INVALID)) {
        g_printerr ("Unable to register type: %s\n", mediatype);
        return NULL;
    }

    self->priv->mo_proxy = dbus_g_proxy_new_for_name (self->priv->conn,
        "org.gnome.GMediaDB", new_path, "org.gnome.GMediaDB.MediaObject");
    g_free (new_path);

    if (!dbus_g_proxy_call (self->priv->mo_proxy, "ref", NULL,
        G_TYPE_INVALID, G_TYPE_INVALID)) {
        g_printerr ("Unable to ref MediaObject: %s\n", mediatype);
        return NULL;
    }

    gchar *path = g_strdup_printf ("%s/gmediadb/%s.db", g_get_user_config_dir (), self->priv->mtype);
    FILE *fptr = fopen (path, "r");
    if (!fptr) {
        //TODO: Read data from file
        fclose (fptr);
    } else {
        //TODO: Create empty table
    }

    dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_added",
        G_TYPE_UINT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_updated",
        G_TYPE_UINT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_removed",
        G_TYPE_UINT, G_TYPE_INVALID);

    dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_added",
        G_CALLBACK (media_added_cb), self, NULL);
    dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_updated",
        G_CALLBACK (media_updated_cb), self, NULL);
    dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_removed",
        G_CALLBACK (media_removed_cb), self, NULL);

    return self;
}

gchar**
gmediadb_get_tags (GMediaDB *self)
{
    return NULL;
}

GPtrArray*
gmediadb_get_entries (GMediaDB *self, GArray *ids, gchar *tags[])
{
    return NULL;
}

gchar**
gmediadb_get_entry (GMediaDB *self, guint id, gchar *tags[])
{
    return NULL;
}

GPtrArray*
gmediadb_get_all_entries (GMediaDB *self, gchar *tags[])
{
    return NULL;
}

gboolean
gmediadb_add_entry (GMediaDB *self, gchar *tags[], gchar *vals[])
{
    return FALSE;
}

gboolean
gmediadb_update_entry (GMediaDB *self, guint id, gchar *tags[], gchar *vals[])
{
    return FALSE;
}

gboolean
gmediadb_remove_entry (GMediaDB *self, guint id)
{
    return FALSE;
}
