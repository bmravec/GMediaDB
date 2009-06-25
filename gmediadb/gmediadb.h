/*
 *      gmediadb.h
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

#ifndef __GMEDIADB_H__
#define __GMEDIADB_H__

#include <glib-object.h>

#define GMEDIADB_TYPE (gmediadb_get_type ())
#define GMEDIADB(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), GMEDIADB_TYPE, GMediaDB))
#define GMEDIADB_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GMEDIADB_TYPE, GMediaDBClass))
#define IS_GMEDIADB(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), GMEDIADB_TYPE))
#define IS_GMEDIADB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GMEDIADB_TYPE))
#define GMEDIADB_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GMEDIADB_TYPE, GMediaDBClass))

G_BEGIN_DECLS

typedef struct _GMediaDB GMediaDB;
typedef struct _GMediaDBClass GMediaDBClass;
typedef struct _GMediaDBPrivate GMediaDBPrivate;

struct _GMediaDB {
    GObject parent;
    
    GMediaDBPrivate *priv;
};

struct _GMediaDBClass {
    GObjectClass parent;
};


GMediaDB *gmediadb_new (const gchar *mediatype);
GType gmediadb_get_type (void);

gchar **gmediadb_get_entry (GMediaDB *self, guint id, gchar *tags[]);
GPtrArray *gmediadb_get_entries (GMediaDB *self, GArray *ids, gchar *tags[]);
GPtrArray *gmediadb_get_all_entries (GMediaDB *self, gchar *tags[]);

G_END_DECLS

#endif /* __GMEDIADB_H__ */

