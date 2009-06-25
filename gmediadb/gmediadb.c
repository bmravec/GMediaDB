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

#include "gmediadb.h"

#include <dbus/dbus-glib.h>
#include <sqlite3.h>

G_DEFINE_TYPE(GMediaDB, gmediadb, G_TYPE_OBJECT)

struct _GMediaDBPrivate {
    DBusGProxy *db_proxy;
    DBusGProxy *mo_proxy;
    DBusGConnection *conn;
    
    gchar *mtype;
    
    sqlite3 *db;
    
    gint op_type;
    guint rowid;
};

static guint signal_add;
static guint signal_remove;

void
media_added_cb (DBusGProxy *proxy, guint id, gpointer user_data)
{
    g_signal_emit (G_OBJECT (user_data), signal_add, 0, id);
}

void
media_removed_cb (DBusGProxy *proxy, guint id, gpointer user_data)
{
    g_signal_emit (G_OBJECT (user_data), signal_remove, 0, id);
}

void
sqlite_update_callback (GMediaDB *self,
                        gint op_type,
                        gchar *database,
                        gchar *table,
                        sqlite3_int64 rowid)
{
/*    if (!dbus_g_proxy_call (self->priv->mo_proxy, "add_entry", NULL,
        G_TYPE_UINT, rowid, G_TYPE_INVALID, G_TYPE_INVALID)) {
        g_printerr ("Unable to send add MediaObject: %d\n", rowid);
    } */
    self->priv->op_type = op_type;
    self->priv->rowid = rowid;
}

static int
sqlite_callback (GPtrArray *array, int argc, char **argv, char **azColName)
{
    int i;
    if (!array) {
        return 0;
    }
    
    gchar **entry = g_new0 (gchar*, argc);
    
    for (i = 0; i < argc; i++) {
        entry[i] = g_strdup (argv[i]);
    }
    
    g_ptr_array_add (array, entry);
    return 0;
}

static gchar*
guint_array_to_string (GArray *nums)
{
    GString *str = g_string_new ("");
    int i;
    
    if (nums->len > 0) {
        g_string_append_printf (str, "%d", g_array_index (nums, guint, 0));
    }
    
    for (i = 1; i < nums->len; i++) {
        g_string_append_printf (str, ",%d", g_array_index (nums, guint, i));
    }
    
    gchar *ret_str = str->str;
    g_string_free (str, FALSE);
    return ret_str;
}

static GPtrArray*
sqlite_get (GMediaDB *self, GArray *ids, gchar *tags[])
{
    GPtrArray *array = g_ptr_array_new ();
    gchar *stmt = NULL;
    
    gchar *stags = g_strjoinv (",", tags);
    
    if (ids) {
        gchar *sids = guint_array_to_string (ids);
        stmt = g_strdup_printf ("SELECT %s FROM music WHERE id in (%s);",
            stags, sids);
        g_free (sids);
    } else {
        stmt = g_strdup_printf ("SELECT %s FROM music;", stags);
    }
    
    g_free (stags);
    g_print ("STMT: %s\n", stmt);
    
    if (self->priv->db) {
        gchar *errmsg;
        
        int rv = sqlite3_exec (self->priv->db, stmt,
            (int (*)(void*,int,char**,char**)) sqlite_callback, array, &errmsg);
        
        if (errmsg) {
            g_print ("ERROR(%d): %s\n", rv, errmsg);
            sqlite3_free (errmsg);
        }
    }
    
    if (stmt) {
        g_free (stmt);
    }
    
    return array;
}

static void
gmediadb_finalize (GObject *object)
{
    GMediaDB *self = GMEDIADB (object);
    
    if (self->priv->db) {
        sqlite3_close (self->priv->db);
        self->priv->db = NULL;
    }
    
    if (self->priv->mo_proxy) {
        dbus_g_proxy_call (self->priv->mo_proxy, "unref", NULL,
            G_TYPE_INVALID, G_TYPE_INVALID);
        
        self->priv->mo_proxy = NULL;
    }
    
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
}

static void
gmediadb_init (GMediaDB *self)
{
    self->priv = G_TYPE_INSTANCE_GET_PRIVATE((self), GMEDIADB_TYPE, GMediaDBPrivate);
    
    int rc = sqlite3_open ("/home/bmravec/.gnome2/gmedia.db", &self->priv->db);
    if(rc){
        g_print ("Can't open database: %s\n", sqlite3_errmsg(self->priv->db));
        sqlite3_close (self->priv->db);
        self->priv->db = NULL;
    } else {
        sqlite3_update_hook (self->priv->db, sqlite_update_callback, self);
    }
    
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
    
    dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_added",
        G_TYPE_UINT, G_TYPE_INVALID);
    dbus_g_proxy_add_signal (self->priv->mo_proxy, "media_removed",
        G_TYPE_UINT, G_TYPE_INVALID);
    
    dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_added",
        G_CALLBACK (media_added_cb), self, NULL);
    dbus_g_proxy_connect_signal (self->priv->mo_proxy, "media_removed",
        G_CALLBACK (media_removed_cb), self, NULL);
    
    return self;
}

GPtrArray*
gmediadb_get_entries (GMediaDB *self, GArray *ids, gchar *tags[])
{
    GPtrArray *entries;
    GError *error = NULL;
    
    g_print ("gmediadb_get_entries\n");
    
    return sqlite_get (self, ids, tags);
}

static int
callback_get_entry (GPtrArray *array, int argc, char **argv, char **azColName)
{
    int i;
    for (i = 0; i < argc; i++) {
        g_ptr_array_add (array, g_strdup (argv[i]));
    }
    
    return 0;
}

gchar**
gmediadb_get_entry (GMediaDB *self, guint id, gchar *tags[])
{
    GPtrArray *array = g_ptr_array_new ();
    gchar *stmt = NULL;
    gchar **values = NULL;
    
    if (self->priv->db) {
        gchar *stags = g_strjoinv (",", tags);
        stmt = g_strdup_printf ("SELECT %s FROM music WHERE id=%d;", stags, id);
        g_free (stags);
        
        gchar *errmsg;
        
        int rv = sqlite3_exec (self->priv->db, stmt,
            (int (*)(void*,int,char**,char**)) callback_get_entry, array, &errmsg);
        
        g_free (stmt);
        
        if (errmsg) {
            g_print ("ERROR(%d): %s\n", rv, errmsg);
            sqlite3_free (errmsg);
        } else {
            values = (gchar**) array->pdata;
            g_ptr_array_free (array, FALSE);
        }
    }
    
    return values;
}

GPtrArray*
gmediadb_get_all_entries (GMediaDB *self, gchar *tags[])
{
    GPtrArray *entries;
    GError *error = NULL;
    
    g_print ("gmediadb_get_all_entries\n");

    return sqlite_get (self, NULL, tags);
}

static gchar*
gcharstar_to_string (gchar *nums[])
{
    GString *str = g_string_new ("");
    gint i;
    
    if (nums[0]) {
        g_string_append_printf (str, "\"%s\"", nums[0]);
    }
    
    for (i = 1; nums[i]; i++) {
        g_string_append_printf (str, ",\"%s\"", nums[i]);
    }
    
    gchar *ret_str = str->str;
    g_string_free (str, FALSE);
    return ret_str;
}

GPtrArray*
gmediadb_set_entry (GMediaDB *self, gchar *tags[], gchar *vals[])
{
    gchar *errmsg;
    gchar *stag = g_strjoinv (",", tags);
    gchar *sval = gcharstar_to_string (vals);
    
    g_print ("TAGS: (%s)\n", stag);
    g_print ("VALS: (%s)\n", sval);
    
    gchar *stmt = g_strdup_printf ("INSERT INTO music (%s) VALUES (%s);",
        stag, sval);
    
    g_free (stag);
    g_free (sval);
    
    g_print ("STMT: %s\n", stmt);
    
    int rv = sqlite3_exec (self->priv->db, stmt, NULL, NULL, &errmsg);
    
    g_free (stmt);
    
    if (errmsg) {
        g_print ("ERROR(%d): %s\n", rv, errmsg);
        sqlite3_free (errmsg);
        return;
    }
    
    if (!dbus_g_proxy_call (self->priv->mo_proxy, "add_entry", NULL,
        G_TYPE_UINT, self->priv->rowid, G_TYPE_INVALID, G_TYPE_INVALID)) {
        g_printerr ("Unable to send add MediaObject: %d\n", self->priv->rowid);
    }
}

