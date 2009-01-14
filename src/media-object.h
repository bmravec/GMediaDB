/*
 *      media-object.h
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
 
#ifndef _MEDIA_OBJECT_H_
#define _MEDIA_OBJECT_H_

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gmediadb.h"
        
#define MEDIA_OBJECT_TYPE (media_object_get_type ())
#define MEDIA_OBJECT(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), MEDIA_OBJECT_TYPE, MediaObject))
#define MEDIA_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MEDIA_OBJECT_TYPE, MediaObjectClass))
#define IS_MEDIA_OBJECT(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), MEDIA_OBJECT_TYPE))
#define IS_MEDIA_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MEDIA_OBJECT_TYPE))
#define MEDIA_OBJECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MEDIA_OBJECT_TYPE, MediaObjectClass))

G_BEGIN_DECLS

typedef struct _MediaObject MediaObject;
typedef struct _MediaObjectClass MediaObjectClass;

struct _MediaObject {
        GObject parent;
};

struct _MediaObjectClass {
        GObjectClass parent;
        
        void (*media_added) (MediaObject *mo, guint ident);
        void (*media_removed) (MediaObject *mo, guint ident);
};

MediaObject *media_object_new (DBusGConnection *conn, gchar *media_type, GMediaDB *gdb);
GType media_object_get_type (void);

gboolean media_object_ref (MediaObject *self, GError **error);
gboolean media_object_unref (MediaObject *self, GError **error);
gboolean media_object_get_entries (MediaObject *self, GArray *ids, gchar **tags, GPtrArray **entries, GError **error);
gboolean media_object_get_all_entries (MediaObject *self, gchar **tags, GPtrArray **entries, GError **error);
gboolean media_object_import_path (MediaObject *self, gchar *path, GError **error);
gboolean media_object_remove_entries (MediaObject *self, GArray *ids, GError **error);

G_END_DECLS

#endif
