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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <errno.h>

#include <semaphore.h>

#include <sys/ipc.h>
#include <sys/sem.h>

#include "gmediadb.h"

#include <dbus/dbus-glib.h>

G_DEFINE_TYPE(GMediaDB, gmediadb, G_TYPE_OBJECT)

struct _GMediaDBPrivate {
    DBusGProxy *db_proxy;
    DBusGProxy *mo_proxy;
    DBusGConnection *conn;

    GHashTable *table;

    gchar *fpath;

    sem_t *fm, *am;

    int nid;

    gchar *mtype;

    GStringChunk *sc;
};

static guint signal_add;
static guint signal_update;
static guint signal_remove;

void
write_entry (int fd, gint id, GHashTable *table)
{
    GList *k, *ki, *v, *vi;
    gint i, size;
    gchar *str;

    size = g_hash_table_size (table);
    k = g_hash_table_get_keys (table);
    v = g_hash_table_get_values (table);

    write (fd, &id, sizeof (gint));
    write (fd, &size, sizeof (gint));

    for (ki = k, vi = v; ki; ki = ki->next, vi = vi->next) {
        str = (gchar*) ki->data;
        for (i = 0; str[i]; i++);
        write (fd, &i, sizeof (gint));
        write (fd, str, i);

        str = (gchar*) vi->data;
        for (i = 0; str[i]; i++);
        write (fd, &i, sizeof (gint));
        write (fd, str, i);
    }
}

GHashTable*
read_entry (int fd, int *id, GStringChunk *sc)
{
    gint len, num, slen;
    gint i, j;

    len = read (fd, id, sizeof (gint));
    if (len <= 0)
        return NULL;

    len = read (fd, &num, sizeof (gint));
    if (len <= 0)
        return NULL;

    GHashTable *info = g_hash_table_new (g_str_hash, g_str_equal);

    while (num-- > 0) {
        len = read (fd, &slen, sizeof (gint));

        gchar *k = g_new0 (gchar, slen + 1);
        len = read (fd, k, slen);

        len = read (fd, &slen, sizeof (gint));

        gchar *v = g_new0 (gchar, slen + 1);
        len = read (fd, v, slen);

        g_hash_table_insert (info,
            g_string_chunk_insert_const (sc, k),
            g_string_chunk_insert_const (sc, v));

        g_free (k);
        g_free (v);
    }

    return info;
}

void
media_added_cb (DBusGProxy *proxy, guint id, GHashTable *info, GMediaDB *self)
{
    GList *ki, *vi;

    vi = g_hash_table_get_values (info);
    ki = g_hash_table_get_keys (info);

    GHashTable *nentry = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

    for (; ki && vi; ki = ki->next, vi = vi->next) {
        g_hash_table_insert (nentry,
            g_string_chunk_insert_const (self->priv->sc, (gchar*) ki->data),
            g_string_chunk_insert_const (self->priv->sc, (gchar*) vi->data));
    }

    gint *nid = g_new0 (gint, 1);
    *nid = id;

    g_hash_table_insert (self->priv->table, nid, nentry);

    g_signal_emit (self, signal_add, 0, id);
}

void
media_updated_cb (DBusGProxy *proxy, guint id, GHashTable *info, GMediaDB *self)
{
    GList *ki, *vi;

    ki = g_hash_table_get_keys (info);
    vi = g_hash_table_get_values (info);

    GHashTable *entry = g_hash_table_lookup (self->priv->table, &id);

    if (!entry) {
        return;
    }

    for (; ki; ki = ki->next, vi = vi->next) {
        g_hash_table_insert (entry,
            g_string_chunk_insert_const (self->priv->sc, (gchar*) ki->data),
            g_string_chunk_insert_const (self->priv->sc, (gchar*) vi->data));
    }

    g_signal_emit (self, signal_update, 0, id);
}

void
media_removed_cb (DBusGProxy *proxy, guint id, GMediaDB *self)
{
    GHashTable *entry = g_hash_table_lookup (self->priv->table, &id);

    if (entry) {
        g_hash_table_remove (self->priv->table, &id);
    }

    g_signal_emit (self, signal_remove, 0, id);
}

void
media_flush_cb (DBusGProxy *proxy, GMediaDB *self)
{
    sem_wait (self->priv->fm);

    gboolean completed;
    if (!dbus_g_proxy_call (self->priv->mo_proxy, "has_flush_completed", NULL,
        G_TYPE_INVALID, G_TYPE_BOOLEAN, &completed, G_TYPE_INVALID)) {
        return;
    }

    if (!completed) {
        GList *tk = g_hash_table_get_keys (self->priv->table);
        GList *tv = g_hash_table_get_values (self->priv->table);

        int fd = open (self->priv->fpath, O_CREAT | O_WRONLY | O_TRUNC);

        for (; tk; tk = tk->next, tv = tv->next) {
            write_entry (fd, *((gint*) tk->data), (GHashTable*) tv->data);
        }

        if (!dbus_g_proxy_call (self->priv->mo_proxy, "flush_completed", NULL,
            G_TYPE_INVALID, G_TYPE_INVALID)) {
            g_print ("Send Flush Completed Failed\n");
        }
    }

    sem_post (self->priv->fm);
}

static void
gmediadb_finalize (GObject *object)
{
    GMediaDB *self = GMEDIADB (object);

    media_flush_cb (self->priv->mo_proxy, self);

    dbus_g_proxy_disconnect_signal (self->priv->mo_proxy, "media_added",
       G_CALLBACK (media_added_cb), self);
    dbus_g_proxy_disconnect_signal (self->priv->mo_proxy, "media_updated",
        G_CALLBACK (media_updated_cb), self);
    dbus_g_proxy_disconnect_signal (self->priv->mo_proxy, "media_removed",
        G_CALLBACK (media_removed_cb), self);
    dbus_g_proxy_disconnect_signal (self->priv->mo_proxy, "flush",
        G_CALLBACK (media_flush_cb), self);

    g_hash_table_destroy (self->priv->table);
    self->priv->table = NULL;

    sem_close (self->priv->fm);
    self->priv->fm = NULL;

    sem_close (self->priv->am);
    self->priv->am = NULL;

    if (self->priv->mo_proxy) {
        dbus_g_proxy_call (self->priv->mo_proxy, "unref", NULL,
            G_TYPE_INVALID, G_TYPE_INVALID);
        g_object_unref (self->priv->mo_proxy);
        self->priv->mo_proxy = NULL;
    }

    if (self->priv->db_proxy) {
        g_object_unref (self->priv->db_proxy);
        self->priv->db_proxy = NULL;
    }

    if (self->priv->conn) {
        dbus_g_connection_unref (self->priv->conn);
        self->priv->conn = NULL;
    }

    g_free (self->priv->mtype);
    self->priv->mtype = NULL;

    g_free (self->priv->fpath);
    self->priv->fpath = NULL;

    g_string_chunk_clear (self->priv->sc);
    self->priv->sc = NULL;

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

    self->priv->table = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, (GDestroyNotify) g_hash_table_destroy);
    self->priv->nid = 1;

    self->priv->sc = g_string_chunk_new (5 * 1024);
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

    gchar *astr = g_strdup_printf ("/gmediadb.%s.A", self->priv->mtype);
    self->priv->am = sem_open (astr, 0);
    if (self->priv->am == SEM_FAILED) {
        g_print ("Failed to get access mutex for %s", mediatype);
    }
    g_free (astr);

    gchar *fstr = g_strdup_printf ("/gmediadb.%s.F", self->priv->mtype);
    self->priv->fm = sem_open (fstr, 0);
    if (self->priv->fm == SEM_FAILED) {
        g_print ("Failed to get flush mutex for %s", mediatype);
    }
    g_free (fstr);

    sem_wait (self->priv->am);

    if (!dbus_g_proxy_call (self->priv->mo_proxy, "flush_store", NULL, G_TYPE_INVALID, G_TYPE_INVALID)) {
        g_print ("Flush Store Failed\n");
        return NULL;
    }

    //TODO: another way to do this?
    gboolean finished = FALSE;
    while (!finished) {
        if (!dbus_g_proxy_call (self->priv->mo_proxy, "has_flush_completed", NULL,
            G_TYPE_INVALID, G_TYPE_BOOLEAN, &finished, G_TYPE_INVALID)) {
            return NULL;
        }
        if (!finished) {
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000 };
            nanosleep (&ts, NULL);
        }
    }

    int fd = open (self->priv->fpath, O_CREAT | O_RDONLY, 0644);
    if (fd != -1) {
        GHashTable *info;
        gint rid;
        while ((info = read_entry (fd, &rid, self->priv->sc)) != NULL) {
            gint *id = g_new0 (gint, 1);
            *id = rid;
            g_hash_table_insert (self->priv->table, id, info);
        }

        dbus_g_object_register_marshaller (g_cclosure_marshal_VOID__UINT_POINTER,
            G_TYPE_NONE, G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE, G_TYPE_INVALID);

        dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_added",
            G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_updated",
            G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_removed",
            G_TYPE_UINT, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (self->priv->mo_proxy, "flush", G_TYPE_INVALID);

        dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_added",
            G_CALLBACK (media_added_cb), self, NULL);
        dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_updated",
            G_CALLBACK (media_updated_cb), self, NULL);
        dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_removed",
            G_CALLBACK (media_removed_cb), self, NULL);
        dbus_g_proxy_connect_signal (self->priv->mo_proxy, "flush",
            G_CALLBACK (media_flush_cb), self, NULL);

        close (fd);
    } else {
        g_print ("Init Error Occured\n");
    }

    sem_post (self->priv->am);

    //TODO: Set nid correctly or perform scan on insertion into table,
    //      should probably remove variable and do a check.

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

    return array;
}

gchar**
gmediadb_get_entry (GMediaDB *self, guint id, gchar *tags[])
{
    gint num_keys = 0, j;
    while (tags[num_keys++]);
    num_keys--;

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

    sem_wait (self->priv->am);

    gint *nid = g_new0 (gint, 1);
    *nid = self->priv->nid++;

    GError *err = NULL;
    if (!dbus_g_proxy_call (self->priv->mo_proxy, "add_entry", &err,
        G_TYPE_UINT, *nid,
        DBUS_TYPE_G_STRING_STRING_HASHTABLE, nentry,
        G_TYPE_INVALID,
        G_TYPE_INVALID)) {
        g_printerr ("Unable to send add MediaObject: %d: %s\n", *nid, err->message);
        g_error_free (err);
        err = NULL;
    }

    sem_post (self->priv->am);

    g_hash_table_unref (nentry);

    return TRUE;
}

gboolean
gmediadb_update_entry (GMediaDB *self, guint id, gchar *tags[], gchar *vals[])
{
    GHashTable *entry = g_hash_table_lookup (self->priv->table, &id);

    if (!entry) {
        return FALSE;
    }

    gint i;
    for (i = 0; tags[i]; i++) {
        g_hash_table_insert (entry,
            g_string_chunk_insert_const (self->priv->sc, tags[i]),
            g_string_chunk_insert_const (self->priv->sc, vals[i]));
    }

    sem_wait (self->priv->am);

    GError *err = NULL;
    if (!dbus_g_proxy_call (self->priv->mo_proxy, "update_entry", &err,
        G_TYPE_UINT, id, DBUS_TYPE_G_STRING_STRING_HASHTABLE, entry,
        G_TYPE_INVALID, G_TYPE_INVALID)) {
        g_printerr ("Unable to send update MediaObject: %d: %s\n", id, err->message);
        g_error_free (err);
        err = NULL;
    }

    sem_post (self->priv->am);

    return TRUE;
}

gboolean
gmediadb_remove_entry (GMediaDB *self, guint id)
{
    if (!g_hash_table_remove (self->priv->table, &id)) {
        return FALSE;
    }

    sem_wait (self->priv->am);

    if (!dbus_g_proxy_call (self->priv->mo_proxy, "remove_entry", NULL,
        G_TYPE_UINT, id,
        G_TYPE_INVALID,
        G_TYPE_INVALID)) {
        g_printerr ("Unable to send remove MediaObject: %d\n", id);
    }

    sem_post (self->priv->am);

    return TRUE;
}
