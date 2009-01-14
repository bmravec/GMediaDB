/*
 *      gmediadb-util.c
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

#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>

typedef struct _MediaObject MediaObject;
struct _MediaObject {
	GtkWidget *view;
	GtkListStore *store;

	gint page_number, index;
	
	DBusGProxy *proxy;
};

#define MOS_NUMBER 5
static gchar *mos_names[] = {
	"Music",
	"Videos",
	"TVShows",
	"MusicVideos",
	"Pictures",
};

void window_destroy (GtkWidget *widget, gpointer user_data);
void import_clicked (GtkWidget *widget, gpointer user_data);
void remove_clicked (GtkWidget *widget, gpointer user_data);
void media_added_cb (DBusGProxy *proxy, guint id, gpointer user_data);
void media_removed_cb (DBusGProxy *proxy, guint id, gpointer user_data);

static GtkWidget *window, *notebook;
static GtkWidget *import_button, *remove_button, *quit_button;

static MediaObject mos[MOS_NUMBER];

int
main (int argc, char *argv[])
{
	DBusGConnection *conn;
	
	gtk_init (&argc, &argv);
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	notebook = gtk_notebook_new ();

	GtkWidget *vbox, *hbox;
	
	import_button = gtk_button_new_with_label ("Import Path");
	remove_button = gtk_button_new_with_label ("Remove Selected");
	quit_button = gtk_button_new_with_label ("Quit");
	
	vbox = gtk_vbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 0);
	
	conn = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
	if (!conn) {
		g_printerr ("Failed to open connection to bus\n");
		return 1;
	}
	
	int i;
	for (i = 0; i < MOS_NUMBER; i++) {
		mos[i].store = gtk_list_store_new (2, G_TYPE_INT, G_TYPE_STRING);
		mos[i].view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (mos[i].store));

		GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (mos[i].view));
		gtk_tree_selection_set_mode (sel, GTK_SELECTION_MULTIPLE);

		mos[i].index = i;
		
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("ID", renderer,
			"text", 0, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(mos[i].view), column);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("URL", renderer,
			"text", 1, NULL);
		gtk_tree_view_append_column(GTK_TREE_VIEW(mos[i].view), column);
		
		GtkWidget *label = gtk_label_new (mos_names[i]);
		
		GtkWidget *scroll = gtk_scrolled_window_new (NULL, NULL);
		gtk_container_add (GTK_CONTAINER (scroll), mos[i].view);
		
		mos[i].page_number = gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
			scroll, label);
		
		gchar *path = g_strdup_printf ("/org/gnome/GMediaDB/%s", mos_names[i]);
		mos[i].proxy = dbus_g_proxy_new_for_name (conn,
			"org.gnome.GMediaDB", path,
			"org.gnome.GMediaDB.MediaObject");
		g_free (path);

		if (!dbus_g_proxy_call (mos[i].proxy, "ref", NULL, G_TYPE_INVALID, G_TYPE_INVALID)) {
			g_printerr ("Unable to ref MediaObject\n");
			return 1;
		}

		GPtrArray *entries;
		GError *error = NULL;
		gchar *tags[] = { "location" };
		if (!dbus_g_proxy_call (mos[i].proxy, "get_all_entries", &error,
			G_TYPE_STRV, tags, G_TYPE_INVALID,
			dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRV), &entries, G_TYPE_INVALID)) {
			if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
				g_printerr ("Caught remote method exception %s: %s",
					dbus_g_error_get_name (error), error->message);
			else
				g_printerr ("Error: %s\n", error->message);
			g_error_free (error);
			return 1;
		}
		
		int j;
		GtkTreeIter iter;
		for (j = 0; j < entries->len; j++) {
			gchar **entry = g_ptr_array_index (entries, j);
			gtk_list_store_append (mos[i].store, &iter);
			gtk_list_store_set (mos[i].store, &iter, 0, atoi (entry[0]), 1, entry[1], -1);
		}

		g_ptr_array_free (entries, TRUE);
		
		dbus_g_proxy_add_signal (mos[i].proxy, "media_added", G_TYPE_UINT, G_TYPE_INVALID);
		dbus_g_proxy_add_signal (mos[i].proxy, "media_removed", G_TYPE_UINT, G_TYPE_INVALID);
		
		dbus_g_proxy_connect_signal (mos[i].proxy, "media_added",
			G_CALLBACK (media_added_cb), &mos[i], NULL);
		dbus_g_proxy_connect_signal (mos[i].proxy, "media_removed",
			G_CALLBACK (media_removed_cb), &mos[i], NULL);
			
	}

	gtk_box_pack_start (GTK_BOX (hbox), quit_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_drawing_area_new (), TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), remove_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), import_button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_widget_show_all (GTK_WIDGET (window));

	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (window_destroy), NULL);
	g_signal_connect (G_OBJECT (quit_button), "clicked", G_CALLBACK (window_destroy), NULL);
	g_signal_connect (G_OBJECT (import_button), "clicked", G_CALLBACK (import_clicked), NULL);
	g_signal_connect (G_OBJECT (remove_button), "clicked", G_CALLBACK (remove_clicked), NULL);
	
	gtk_main ();
	
	for (i = 0; i < MOS_NUMBER; i++) {
		if (!dbus_g_proxy_call (mos[i].proxy, "unref", NULL, G_TYPE_INVALID, G_TYPE_INVALID)) {
			g_printerr ("Unable to ref MediaObject\n");
			return 1;
		}
	}
	
	return 0;
}

void
window_destroy (GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
}

void
import_clicked (GtkWidget *widget, gpointer user_data)
{
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new ("Open File", GTK_WINDOW (window),
					GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					"Import", GTK_RESPONSE_ACCEPT,
					NULL);
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;

		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gint pn = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
		int i;
		for (i = 0; i < MOS_NUMBER; i++) {
			if (mos[i].page_number == pn) {
				GError *error = NULL;
				if (!dbus_g_proxy_call (mos[i].proxy, "import_path", &error,
					G_TYPE_STRING, filename, G_TYPE_INVALID, G_TYPE_INVALID)) {
					if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
						g_printerr ("Caught remote method exception %s: %s",
							dbus_g_error_get_name (error), error->message);
					else
						g_printerr ("Error: %s\n", error->message);
					g_error_free (error);
					return;
				}
				
				break;
			}
		}

		g_free (filename);
	}

	gtk_widget_destroy (dialog);
}

void
selected_foreach (GtkTreeModel *model,
				  GtkTreePath *path,
				  GtkTreeIter *iter,
				  gpointer data)
{
	gint val;
	gtk_tree_model_get (model, iter, 0, &val, -1);
	g_array_append_val (data, val);
}

void
remove_clicked (GtkWidget *widget, gpointer user_data)
{
	MediaObject *mo;
	GError *error = NULL;

	int i;
	gint pn = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
	for (i = 0; i < MOS_NUMBER; i++)
		if (mos[i].page_number == pn)
			mo = &mos[i];
	
	GArray *ids = g_array_new (TRUE, TRUE, sizeof (guint));
	GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (mo->view));
	gtk_tree_selection_selected_foreach (sel, selected_foreach, ids);

	if (!dbus_g_proxy_call (mo->proxy, "remove_entries", &error,
			DBUS_TYPE_G_UINT_ARRAY, ids, G_TYPE_INVALID, G_TYPE_INVALID)) {
		if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
			g_printerr ("Caught remote method exception %s: %s",
				dbus_g_error_get_name (error), error->message);
		else
			g_printerr ("Error: %s\n", error->message);
		g_error_free (error);
	}
	
	g_array_free (ids, TRUE);
}

void
media_added_cb (DBusGProxy *proxy, guint id, gpointer user_data)
{
	MediaObject *mo = (MediaObject*) user_data;
	
	GPtrArray *entries;
	GError *error = NULL;
	gchar *tags[] = { "location" };
	GArray *ids = g_array_new (FALSE, TRUE, sizeof (guint));
	g_array_append_val (ids, id);
	
	if (!dbus_g_proxy_call (proxy, "get_entries", &error,
			DBUS_TYPE_G_UINT_ARRAY, ids,
			G_TYPE_STRV, tags,
			G_TYPE_INVALID,
			dbus_g_type_get_collection ("GPtrArray", G_TYPE_STRV), &entries,
			G_TYPE_INVALID)) {
		if (error->domain == DBUS_GERROR && error->code == DBUS_GERROR_REMOTE_EXCEPTION)
			g_printerr ("Caught remote method exception %s: %s",
				dbus_g_error_get_name (error), error->message);
		else
			g_printerr ("Error: %s\n", error->message);
		g_error_free (error);
		return;
	}
		
	int j;
	GtkTreeIter iter;
	for (j = 0; j < entries->len; j++) {
		gchar **entry = g_ptr_array_index (entries, j);
		gtk_list_store_append (mo->store, &iter);
		gtk_list_store_set (mo->store, &iter, 0, atoi (entry[0]), 1, entry[1], -1);
	}

	g_ptr_array_free (entries, TRUE);
}

void
media_removed_cb (DBusGProxy *proxy, guint id, gpointer user_data)
{
	MediaObject *mo = (MediaObject*) user_data;
	GtkTreeIter iter;
	
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (mo->store), &iter);
	do {
		guint num;
		gtk_tree_model_get (GTK_TREE_MODEL (mo->store), &iter, 0, &num, -1);
		
		if (num == id) {
			gtk_list_store_remove (mo->store, &iter);
			return;
		}
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (mo->store), &iter));
}

