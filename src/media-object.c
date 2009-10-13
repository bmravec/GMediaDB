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

#include "media-object.h"
#include "media-object-glue.h"

G_DEFINE_TYPE(MediaObject, media_object, G_TYPE_OBJECT)

#define MEDIA_OBJECT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), MEDIA_OBJECT_TYPE, MediaObjectPrivate))

struct _MediaObjectPrivate {
    gboolean mod;
};

static guint signal_media_added, signal_media_updated, signal_media_removed, signal_flush;

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
        NULL, NULL, g_cclosure_marshal_VOID__UINT_POINTER,
        G_TYPE_NONE, 2, G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE);

    signal_media_updated = g_signal_new ("media_updated", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (MediaObjectClass, media_updated),
        NULL, NULL, g_cclosure_marshal_VOID__UINT_POINTER,
        G_TYPE_NONE, 2, G_TYPE_UINT, DBUS_TYPE_G_STRING_STRING_HASHTABLE);

    signal_media_removed = g_signal_new ("media_removed", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (MediaObjectClass, media_removed),
        NULL, NULL, g_cclosure_marshal_VOID__UINT,
        G_TYPE_NONE, 1, G_TYPE_UINT);

    signal_flush = g_signal_new ("flush", G_TYPE_FROM_CLASS (klass),
        G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

    dbus_g_object_type_install_info (MEDIA_OBJECT_TYPE,
                                     &dbus_glib_media_object_object_info);
}

static void
media_object_init (MediaObject *self)
{
    self->priv = MEDIA_OBJECT_GET_PRIVATE (self);

    self->priv->mod = FALSE;
}

MediaObject *
media_object_new ()
{
    return g_object_new (MEDIA_OBJECT_TYPE, NULL);
}

gboolean
media_object_add_entry (MediaObject *self, guint ident, GHashTable *info, GError **error)
{
    self->priv->mod = TRUE;
    g_signal_emit (G_OBJECT (self), signal_media_added, 0, ident, info);

    return TRUE;
}

gboolean
media_object_update_entry (MediaObject *self, guint ident, GHashTable *info, GError **error)
{
    self->priv->mod = TRUE;
    g_signal_emit (G_OBJECT (self), signal_media_updated, 0, ident, info);

    return TRUE;
}

gboolean
media_object_remove_entry (MediaObject *self, guint ident, GError **error)
{
    self->priv->mod = TRUE;
    g_signal_emit (G_OBJECT (self), signal_media_removed, 0, ident);

    return TRUE;
}

gboolean
media_object_flush_store (MediaObject *self, GError **error)
{
    // If state of file is different than database, flush store to file

    if (self->priv->mod) {
        g_print ("Flush\n");
        g_signal_emit (self, signal_flush, 0);
    }

    return TRUE;
}

gboolean
media_object_flush_completed (MediaObject *self, GError **error)
{
    self->priv->mod = FALSE;

    return TRUE;
}

gboolean
media_object_has_flush_completed (MediaObject *self, gboolean *val, GError **error)
{
    *val = self->priv->mod ? FALSE : TRUE;

    return TRUE;
}
