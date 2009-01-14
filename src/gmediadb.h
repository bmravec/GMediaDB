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

#define GMEDIADB_DBUS_SERVICE "org.gnome.GMediaDB"
#define GMEDIADB_DBUS_PATH "/org/gnome/GMediaDB"

G_BEGIN_DECLS

#define GMEDIA_DB_TYPE (gmedia_db_get_type())
#define GMEDIA_DB(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GMEDIA_DB_TYPE, GMediaDB))
#define GMEDIA_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GMEDIA_DB_TYPE, GMediaDBClass))
#define IS_GMEDIA_DB(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GMEDIA_DB_TYPE))
#define IS_GMEDIA_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), GMEDIA_DB_TYPE))

typedef struct _GMediaDB GMediaDB;
typedef struct _GMediaDBClass GMediaDBClass;

struct _GMediaDB
{
	GObject parent;
};

struct _GMediaDBClass
{
	GObjectClass parent_class;
};

GType gmedia_db_get_type (void);
GObject *gmedia_db_new (void);

void gmedia_db_run (GMediaDB *self);
void gmedia_db_stop (GMediaDB *self);

void gmedia_db_ref (GMediaDB *self);
void gmedia_db_unref (GMediaDB *self);

G_END_DECLS

#endif /* __GMEDIADB_H__ */

