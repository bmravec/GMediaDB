/*
 *      media-object.c
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
#include "media-object.h"
#include "media-object-glue.h"

G_DEFINE_TYPE(MediaObject, media_object, G_TYPE_OBJECT)

#define MEDIA_OBJECT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), MEDIA_OBJECT_TYPE, MediaObjectPrivate))

struct _MediaObjectPrivate {
    gchar *media_type;
    GMediaDB *gdb;
    gint ref_cnt;
};

static guint signal_media_added, signal_media_updated, signal_media_removed;

static void
media_object_finalize (GObject *object)
{
    G_OBJECT_CLASS (media_object_parent_class)->finalize (object);
}

static void
media_object_class_init (MediaObjectClass *klass)
{
    GObjectClass *object_class;
    object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private ((gpointer) klass, sizeof (MediaObjectPrivate));

    object_class->finalize = media_object_finalize;

    signal_media_added = g_signal_new ("media_added", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (MediaObjectClass, media_added),
        NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 2, G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE);

    signal_media_updated = g_signal_new ("media_updated", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (MediaObjectClass, media_updated),
        NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 2, G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE);

    signal_media_removed = g_signal_new ("media_removed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (MediaObjectClass, media_removed),
        NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);

    dbus_g_object_type_install_info (MEDIA_OBJECT_TYPE,
                                     &dbus_glib_media_object_object_info);
}

static void
media_object_init (MediaObject *self)
{
    self->priv = MEDIA_OBJECT_GET_PRIVATE (self);

    self->priv->ref_cnt = 0;
}

MediaObject *
media_object_new (DBusGConnection *conn, gchar *media_type, GMediaDB *gdb)
{
    MediaObject *self = g_object_new (MEDIA_OBJECT_TYPE, NULL);

    self->priv->gdb = gdb;
    self->priv->media_type = g_strdup (media_type);

    gchar *path = g_strdup_printf ("%s/%s", GMEDIADB_DBUS_PATH, media_type);
    dbus_g_connection_register_g_object (conn, path, G_OBJECT (self));
    g_free (path);

    return self;
}

gboolean
media_object_add_entry (MediaObject *self, guint ident, GHashTable *info, GError **error)
{
    g_signal_emit (G_OBJECT (self), signal_media_added, 0, ident, info);
}

gboolean
media_object_update_entry (MediaObject *self, guint ident, GHashTable *info, GError **error)
{
    g_signal_emit (G_OBJECT (self), signal_media_updated, 0, ident, info);
}

gboolean
media_object_remove_entry (MediaObject *self, guint ident, GError **error)
{
    g_signal_emit (G_OBJECT (self), signal_media_removed, 0, ident);
}

gboolean
media_object_ref (MediaObject *self,
                  GError **error)
{
    MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);

    gmediadb_ref (priv->gdb);

    priv->ref_cnt += 1;

    return TRUE;
}

gboolean
media_object_unref (MediaObject *self,
                    GError **error)
{
    MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);
    priv->ref_cnt--;

    gmediadb_unref (priv->gdb);
    return TRUE;
}
