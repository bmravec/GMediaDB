/*
 *      media-object.c
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
 
#include "gmediadb.h"
#include "media-object.h"
#include "media-object-glue.h"
#include "tag-handler.h"

#include <stdio.h>
#include <libxml/parser.h>

G_DEFINE_TYPE(MediaObject, media_object, G_TYPE_OBJECT)

typedef struct _MediaObjectPrivate MediaObjectPrivate;

#define MEDIA_OBJECT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), MEDIA_OBJECT_TYPE, MediaObjectPrivate))

typedef enum {
	LOADER_START = 0,
	LOADER_GET_DB,
	LOADER_GET_ENTRY,
	LOADER_GET_TAG
} LoaderState;

struct _MediaObjectPrivate {
	gchar *media_type;
	gchar *media_file;
	GMediaDB *gdb;
	GHashTable *media;
	guint next_id;
	gint ref_cnt;
	TagHandler *tag_handler;

	// XML Parser Data
	xmlSAXHandlerPtr sax;
	GHashTable *entry;
	LoaderState state;
	gchar *key, *val;
};

static void media_object_start_element (MediaObject *self, const char *name, const char **attrs);
static void media_object_end_element (MediaObject *self, const char *name);
static void media_object_characters (MediaObject *self, const char *chars, int len);

static guint signal_media_added, signal_media_removed;

static void
media_object_load_xml (MediaObject *self)
{
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);
	
	if (priv->media != NULL) {
		g_hash_table_unref (priv->media);
	}
	
	priv->media = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, g_hash_table_unref);
	
	priv->sax = g_new0 (xmlSAXHandler, 1);
	
	priv->sax->startElement = (startElementSAXFunc) media_object_start_element;
	priv->sax->endElement = (endElementSAXFunc) media_object_end_element;
	priv->sax->characters = (charactersSAXFunc) media_object_characters;

	priv->state = LOADER_START;
	priv->key = NULL;
	priv->val = NULL;
	priv->entry = NULL;
	
	if (!g_file_test (priv->media_file, G_FILE_TEST_EXISTS))
		return;
	
	printf ("Parsing XML in file %s\n", priv->media_file);
	xmlSAXUserParseFile (priv->sax, self, priv->media_file);
		
	g_free (priv->sax);
	priv->sax = NULL;
}

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
		G_TYPE_NONE, 1, G_TYPE_UINT);

	signal_media_removed = g_signal_new ("media_removed", G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST, G_STRUCT_OFFSET (MediaObjectClass, media_removed),
		NULL, NULL, g_cclosure_marshal_VOID__UINT,
		G_TYPE_NONE, 1, G_TYPE_UINT);

	dbus_g_object_type_install_info (MEDIA_OBJECT_TYPE,
									 &dbus_glib_media_object_object_info);
}

static void
media_object_init (MediaObject *object)
{
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (object);
	
	priv->next_id = 1;
	priv->ref_cnt = 0;	
}

MediaObject *
media_object_new (DBusGConnection *conn, gchar *media_type, GMediaDB *gdb)
{
	MediaObject *object = g_object_new (MEDIA_OBJECT_TYPE, NULL);
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (object);

	priv->gdb = gdb;
	priv->media_file = g_strdup_printf ("%s/%s.xml", "/home/brett/.gnome2/gmediadb", media_type);
	
	priv->media_type = g_strdup (media_type);
	
	gchar *path = g_strdup_printf ("%s/%s", GMEDIADB_DBUS_PATH, media_type);
	dbus_g_connection_register_g_object (conn, path, object);
	g_free (path);

	return object;
}

gboolean
media_object_ref (MediaObject *self,
				  GError **error)
{
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);
	printf ("Media_Object_ref\n");
	
	gmedia_db_ref (priv->gdb);
	
	priv->ref_cnt += 1;

	if (priv->ref_cnt == 1) {
		media_object_load_xml (self);
		priv->ref_cnt = 1;
		priv->tag_handler = tag_handler_new (self);
	}

	return TRUE;
}

gboolean
media_object_unref (MediaObject *self,
					GError **error)
{
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);
	printf ("Media_Object_unref (%d)\n", priv->ref_cnt);
	
	priv->ref_cnt--;
	
	if (priv->ref_cnt <= 0) {
		priv->ref_cnt = 0;

		g_object_unref (G_OBJECT (priv->tag_handler));
		priv->tag_handler = NULL;

		printf ("MediaObject<%s> write to disk\n", priv->media_type);
	}
	
	gmedia_db_unref (priv->gdb);
	return TRUE;
}

gboolean
media_object_get_entries (MediaObject *self,
						  GArray *ids,
						  gchar **tags,
						  GPtrArray **entries,
						  GError **error)
{
	printf ("Media_Object_get_entries\n");
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);

	GHashTableIter iter;

	GHashTable *old_entry;
	guint *id;
	
	gchar *tag_val;
	
	*entries = g_ptr_array_new ();
	gchar **ret_entry;

	gint tag_len = 2;
	while (tags[tag_len]) tag_len++;

	int k;
	for (k = 0; k < ids->len; k++) {
		old_entry = g_hash_table_lookup (priv->media, &k);
		
		int i = 0, j = 1;
		
		ret_entry = g_new0 (gchar*, tag_len+1);
		ret_entry[0] = g_strdup (g_hash_table_lookup (old_entry, "id"));
		
		while (tags[i]) {
			tag_val = g_hash_table_lookup (old_entry, tags[i]);

			if (tag_val != NULL)
				ret_entry[j] = g_strdup (tag_val);
			else
				ret_entry[j] = g_strdup ("");
			
			j++;
			i++;
		}
		
		g_ptr_array_add (*entries, ret_entry);
	}

	return TRUE;
}

gboolean
media_object_get_all_entries (MediaObject *self,
							  gchar **tags,
							  GPtrArray **entries,
							  GError **error)
{
	printf ("Media_Object_get_all_entries\n");
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);

	GHashTableIter iter;

	GHashTable *old_entry;
	guint *id;
	
	gchar *tag_val;
	
	*entries = g_ptr_array_new ();
	gchar **ret_entry;

	gint tag_len = 2;
	while (tags[tag_len]) tag_len++;
	
	g_hash_table_iter_init (&iter, priv->media);
	while (g_hash_table_iter_next (&iter, &id, &old_entry)) {
		int i = 0, j = 1;
		
		ret_entry = g_new0 (gchar*, tag_len+1);
		ret_entry[0] = g_strdup (g_hash_table_lookup (old_entry, "id"));
		
		while (tags[i]) {
			tag_val = g_hash_table_lookup (old_entry, tags[i]);

			if (tag_val != NULL)
				ret_entry[j] = g_strdup (tag_val);
			else
				ret_entry[j] = g_strdup ("");
			
			j++;
			i++;
		}
		
		g_ptr_array_add (*entries, ret_entry);
	}

	return TRUE;
}

gboolean
media_object_import_path (MediaObject *self,
						  gchar *path,
						  GError **error)
{
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);
	printf ("Media_Object_import: %s\n", path);
	
	
	tag_handler_add_entry (priv->tag_handler, path);
	
	return TRUE;
}

gboolean
media_object_remove_entries (MediaObject *self,
							 GArray *ids,
							 GError **error)
{
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);
	printf ("media_object_remove_entries\n");
	
	int i;
	for (i = 0; i < ids->len; i++) {
		guint index = g_array_index (ids, guint, i);
	
		g_print ("Removing Entry %d\n", index);

		g_signal_emit (self, signal_media_removed, NULL, index);

		g_hash_table_remove (priv->media, &index);
	}
	
	return TRUE;
}

static void
media_object_start_element (MediaObject *self,
							const char *name,
							const char **attrs)
{
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);
	
	switch (priv->state) {
		case LOADER_START:
			priv->state = LOADER_GET_DB;
			break;
		case LOADER_GET_DB:
			priv->entry = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
			priv->state = LOADER_GET_ENTRY;
			break;
		case LOADER_GET_ENTRY:
			priv->key = g_strdup (name);
			priv->state = LOADER_GET_TAG;
			priv->val = g_strdup ("");
			break;
		case LOADER_GET_TAG:
			break;
	};
}

static void
media_object_end_element (MediaObject *self,
						  const char *name)
{
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);
	gint *id;
		
	switch (priv->state) {
		case LOADER_GET_DB:
			priv->state = LOADER_START;
			break;
		case LOADER_GET_ENTRY:
			id = g_new0 (gint, 1);
			*id = priv->next_id++;
			
			g_hash_table_insert (priv->entry, g_strdup ("id"), g_strdup_printf ("%d",*id));
			g_hash_table_insert (priv->media, id, priv->entry);
			priv->entry = NULL;

			priv->state = LOADER_GET_DB;
			break;
		case LOADER_GET_TAG:
			g_hash_table_insert (priv->entry, priv->key, priv->val);

			priv->key = NULL;
			priv->val = NULL;
			
			priv->state = LOADER_GET_ENTRY;
			break;
	};
}

static void
media_object_characters (MediaObject *self,
						 const char *chars,
						 int len)
{
	MediaObjectPrivate *priv = MEDIA_OBJECT_GET_PRIVATE (self);
	
	if (priv->state == LOADER_GET_TAG) {
		gchar *new_part = g_strndup (chars, len);
		gchar *tstr = g_strconcat (priv->val, new_part, NULL);

		g_free (priv->val);
		g_free (new_part);
		
		priv->val = tstr;
	}
}

