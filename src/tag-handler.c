/*
 *      tag-handler.c
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

#include "tag-handler.h"
#include "gmediadb-marshal.h"

#include <tag_c.h>

G_DEFINE_TYPE(TagHandler, tag_handler, G_TYPE_OBJECT)

static guint signal_add_entry;

typedef struct _TagHandlerPrivate TagHandlerPrivate;

#define TAG_HANDLER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), TAG_HANDLER_TYPE, TagHandlerPrivate))

struct _TagHandlerPrivate
{
	GAsyncQueue *job_queue;
	GThread *job_thread;
	MediaObject *mo;

	gboolean run;
};

void tag_handler_emit_add_signal (TagHandler *self, GHashTable *info);

gpointer
tag_handler_main (gpointer data)
{
	TagHandler *self = TAG_HANDLER (data);
	TagHandlerPrivate *priv = TAG_HANDLER_GET_PRIVATE (self);
	GAsyncQueue *queue = priv->job_queue;
	
	const TagLib_AudioProperties *properties;
	TagLib_File *file;
	TagLib_Tag *tag;
	GHashTable *info;
	
	gboolean has_ref = FALSE;
	
	taglib_set_strings_unicode(TRUE);

	while (priv->run) {
		gchar *entry;

		if (has_ref && g_async_queue_length (priv->job_queue) == 0) {
			media_object_unref (priv->mo, NULL);
			has_ref = FALSE;
		}
		
		entry = g_async_queue_pop (priv->job_queue);
		
		if (!entry || !priv->run)
			continue;
		
		if (!has_ref) {
			media_object_ref (priv->mo, NULL);
			has_ref = TRUE;
		}
		
		info = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
		
		file = taglib_file_new(entry);
		tag = taglib_file_tag(file);
		properties = taglib_file_audioproperties(file);
		
		g_hash_table_insert (info, g_strdup ("artist"), g_strdup (taglib_tag_artist (tag)));
		g_hash_table_insert (info, g_strdup ("title"), g_strdup (taglib_tag_title (tag)));
		g_hash_table_insert (info, g_strdup ("album"), g_strdup (taglib_tag_album (tag)));
		g_hash_table_insert (info, g_strdup ("comment"), g_strdup (taglib_tag_comment (tag)));
		g_hash_table_insert (info, g_strdup ("genre"), g_strdup (taglib_tag_genre (tag)));
		g_hash_table_insert (info, g_strdup ("year"), g_strdup_printf ("%d",taglib_tag_year (tag))); 
		g_hash_table_insert (info, g_strdup ("track"), g_strdup_printf ("%d",taglib_tag_track (tag))); 
	
		tag_handler_emit_add_signal (self, info);

		g_hash_table_unref (info);
		info = NULL;
		
		taglib_tag_free_strings();
		taglib_file_free(file);
	}
}

static void
tag_handler_finalize (GObject *object)
{
	TagHandler *self = TAG_HANDLER (object);
	TagHandlerPrivate *priv = TAG_HANDLER_GET_PRIVATE (self);
	
	priv->run = FALSE;

	while (g_async_queue_length (priv->job_queue) > 0) {
		gchar *str = g_async_queue_try_pop (priv->job_queue);
		if (str) {
			//TODO: DO SOMETHING WITH QUEUED ITEMS
			g_free (str);
		}
	}
	
	g_async_queue_push (priv->job_queue, "");

	g_thread_join (priv->job_thread);
	
	g_async_queue_unref (priv->job_queue);

	G_OBJECT_CLASS (tag_handler_parent_class)->finalize (object);
}

static void
tag_handler_class_init (TagHandlerClass *klass)
{
	GObjectClass *object_class;
	object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private ((gpointer) klass, sizeof (TagHandlerPrivate));

	object_class->finalize = tag_handler_finalize;
	
	signal_add_entry = g_signal_new ("add-entry", G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST, 0, NULL, NULL, &gmediadb_marshal_VOID__POINTER_STRING,
		G_TYPE_NONE, 1, G_TYPE_HASH_TABLE);
}

static void
tag_handler_init (TagHandler *self)
{
	TagHandlerPrivate *priv = TAG_HANDLER_GET_PRIVATE (self);
	
	priv->job_queue = g_async_queue_new ();
	priv->run = TRUE;
	priv->job_thread = g_thread_create (tag_handler_main, self, TRUE, NULL);
}

TagHandler *
tag_handler_new (MediaObject *mo)
{
	TagHandler *th = g_object_new (TAG_HANDLER_TYPE, NULL);
	TagHandlerPrivate *priv = TAG_HANDLER_GET_PRIVATE (th);
	
	priv->mo = mo;
	
	return th;
}

void
tag_handler_emit_add_signal (TagHandler *self, GHashTable *info)
{
	GHashTableIter iter;
	gpointer key, value;
	
//	g_signal_emit (self, signal_add_entry, 0, info);
	g_print ("---------------------------------------------");
	g_print ("Add Entry:\n");
	
	g_hash_table_iter_init (&iter, info);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		g_print ("%s: %s\n", (gchar*) key, (gchar*) value);
	}
	g_print ("---------------------------------------------");
}

void
tag_handler_add_entry (TagHandler *self, gchar *location)
{
	TagHandlerPrivate *priv = TAG_HANDLER_GET_PRIVATE (self);
	
	g_async_queue_push (priv->job_queue, g_strdup (location));
}
