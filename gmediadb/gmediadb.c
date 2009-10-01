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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/file.h>

#include "gmediadb.h"

#include <dbus/dbus-glib.h>

G_DEFINE_TYPE(GMediaDB, gmediadb, G_TYPE_OBJECT)

struct _GMediaDBPrivate {
    DBusGProxy *db_proxy;
    DBusGProxy *mo_proxy;
    DBusGConnection *conn;

    GHashTable *table;

    gchar *fpath;
    int fd;

    int nid;

    gchar *mtype;
};

static guint signal_add;
static guint signal_update;
static guint signal_remove;

void
media_added_cb (DBusGProxy *proxy, guint id, GHashTable *info, GMediaDB *self)
{
    GList *ki, *values;
    GList *vi, *keys;

    values = g_hash_table_get_values (info);
    keys = g_hash_table_get_keys (info);

    GHashTable *nentry = g_hash_table_new (g_str_hash, g_str_equal);

    for (ki = keys, vi = values; ki; ki = ki->next, vi = vi->next) {
        g_hash_table_insert (nentry, g_strdup ((gchar*) ki->data), g_strdup ((gchar*) vi->data));
    }

    gint *nid = g_new0 (gint, 1);
    *nid = id;

    g_hash_table_insert (self->priv->table, nid, nentry);

    g_signal_emit (self, signal_add, 0, id);
}

void
media_updated_cb (DBusGProxy *proxy, guint id, GHashTable *info, GMediaDB *self)
{
    g_signal_emit (self, signal_update, 0, id);
}

void
media_removed_cb (DBusGProxy *proxy, guint id, GMediaDB *self)
{
    g_signal_emit (self, signal_remove, 0, id);
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

    self->priv->table = g_hash_table_new (g_int_hash, g_int_equal);
    self->priv->nid = 1;
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

    self->priv->fpath = g_strdup_printf ("%s/gmediadb/%s.db", g_get_user_config_dir (), self->priv->mtype);

    self->priv->fd = open (self->priv->fpath, O_CREAT | O_RDONLY);
    if (self->priv->fd != -1) {
        //TODO: Read data from file
        flock (self->priv->fd, LOCK_SH);

        dbus_g_object_register_marshaller (g_cclosure_marshal_VOID__UINT_POINTER,
            G_TYPE_NONE, G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE, G_TYPE_INVALID);

        dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_added",
            G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_updated",
            G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_removed",
            G_TYPE_UINT, G_TYPE_INVALID);

        dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_added",
            G_CALLBACK (media_added_cb), self, NULL);
        dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_updated",
            G_CALLBACK (media_updated_cb), self, NULL);
        dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_removed",
            G_CALLBACK (media_removed_cb), self, NULL);

        flock (self->priv->fd, LOCK_UN);
    } else {
        g_print ("Init Error Occured\n");
    }

    return self;
}

gchar**
gmediadb_get_tags (GMediaDB *self)
{
    g_print ("gmediadb_get_tags: stub\n");

    return NULL;
}

GPtrArray*
gmediadb_get_entries (GMediaDB *self, GArray *ids, gchar *tags[])
{
    GPtrArray *array = g_ptr_array_new ();
    gint num_keys = 0;
    while (tags[num_keys++]);
    num_keys--;

    flock (self->priv->fd, LOCK_SH);

    gint i, j;
    for (i = 0; i < ids->len; i++) {
        GHashTable *tentry = g_hash_table_lookup (self->priv->table, &g_array_index (ids, gint, i));

        if (!tentry) {
            continue;
        }

        gchar **entry = g_new0 (gchar*, num_keys);

        for (j = 0; j < num_keys; j++) {
            if (!g_strcmp0 (tags[j], "id")) {
                entry[j] = g_strdup_printf ("%d", g_array_index (ids, gint, i));
            } else {
                entry[j] = g_hash_table_lookup (tentry, tags[j]);
            }
        }

        g_ptr_array_add (array, entry);
    }

    flock (self->priv->fd, LOCK_UN);

    return array;
}

gchar**
gmediadb_get_entry (GMediaDB *self, guint id, gchar *tags[])
{
    gint num_keys = 0, j;
    while (tags[num_keys++]);
    num_keys--;

    flock (self->priv->fd, LOCK_SH);

    GHashTable *tentry = g_hash_table_lookup (self->priv->table, &id);

    if (!tentry) {
        return NULL;
    }

    gchar **entry = g_new0 (gchar*, num_keys);

    for (j = 0; j < num_keys; j++) {
        if (!g_strcmp0 (tags[j], "id")) {
            entry[j] = g_strdup_printf ("%d", id);
        } else {
            entry[j] = g_hash_table_lookup (tentry, tags[j]);
        }
    }

    flock (self->priv->fd, LOCK_UN);

    return entry;
}

GPtrArray*
gmediadb_get_all_entries (GMediaDB *self, gchar *tags[])
{
    GList *iter, *values, *i2, *keys;
    gint i, j, num_keys = 0;

    GPtrArray *array = g_ptr_array_new ();

    while (tags[num_keys++]);
    num_keys--;

    flock (self->priv->fd, LOCK_SH);

    values = g_hash_table_get_values (self->priv->table);
    keys = g_hash_table_get_keys (self->priv->table);

    for (iter = values, i2 = keys; iter; iter = iter->next, i2 = i2->next) {
        GHashTable *tentry = (GHashTable*) iter->data;

        gchar **entry = g_new0 (gchar*, num_keys);

        for (j = 0; j < num_keys; j++) {
            if (!g_strcmp0 (tags[j], "id")) {
                entry[j] = g_strdup_printf ("%d", *((gint*) i2->data));
            } else {
                entry[j] = g_hash_table_lookup (tentry, tags[j]);
            }
        }

        g_ptr_array_add (array, entry);
    }

    flock (self->priv->fd, LOCK_UN);

    return array;
}

gboolean
gmediadb_add_entry (GMediaDB *self, gchar *tags[], gchar *vals[])
{
    GHashTable *nentry = g_hash_table_new (g_str_hash, g_str_equal);

    gint i;
    for (i = 0; tags[i]; i++) {
        g_hash_table_insert (nentry, tags[i], vals[i]);
    }

    flock (self->priv->fd, LOCK_EX);

    gint *nid = g_new0 (gint, 1);
    *nid = self->priv->nid++;

    if (!dbus_g_proxy_call (self->priv->mo_proxy, "add_entry", NULL,
        G_TYPE_UINT, *nid,
        DBUS_TYPE_G_STRING_STRING_HASHTABLE, nentry,
        G_TYPE_INVALID,
        G_TYPE_INVALID)) {
        g_printerr ("Unable to send add MediaObject: %d\n", *nid);
    }

    flock (self->priv->fd, LOCK_UN);

    g_hash_table_unref (nentry);

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
