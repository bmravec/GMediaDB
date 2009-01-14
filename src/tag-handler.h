/*
 *      tag-handler.h
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
 
#ifndef _TAG_HANDLER_H_
#define _TAG_HANDLER_H_

#include <glib-object.h>

#include "media-object.h"

#define TAG_HANDLER_TYPE (tag_handler_get_type ())
#define TAG_HANDLER(object) (G_TYPE_CHECK_INSTANCE_CAST ((object), TAG_HANDLER_TYPE, TagHandler))
#define TAG_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TAG_HANDLER_TYPE, TagHandlerClass))
#define IS_TAG_HANDLER(object) (G_TYPE_CHECK_INSTANCE_TYPE ((object), TAG_HANDLER_TYPE))
#define IS_TAG_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TAG_HANDLER_TYPE))
#define TAG_HANDLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TAG_HANDLER_TYPE, TagHandlerClass))

G_BEGIN_DECLS

typedef struct _TagHandler TagHandler;
typedef struct _TagHandlerClass TagHandlerClass;

struct _TagHandler {
        GObject parent;
};

struct _TagHandlerClass {
        GObjectClass parent;
};

TagHandler *tag_handler_new (MediaObject *gdb);
GType tag_handler_get_type (void);
void tag_handler_add_entry (TagHandler *self, gchar *location);


G_END_DECLS

#endif

