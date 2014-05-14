#include <locale.h>

#include "gnome-pocket.h"
#include "gnome-pocket-item.h"

static GMainLoop *loop = NULL;

static void
print_items (GnomePocket *pocket)
{
	GList *items, *l;

	items = gnome_pocket_get_items (pocket);
	for (l = items; l != NULL; l = l->next) {
		GnomePocketItem *item = l->data;

		gnome_pocket_item_print (item);
	}
}

static void
refresh_cb (GObject *object,
	    GAsyncResult *res,
	    gpointer user_data)
{
	gboolean ret;

	g_message ("finished a refresh");

	ret = gnome_pocket_refresh_finish (GNOME_POCKET (object), res, NULL);
	g_message ("refresh_cb: %d", ret);

	print_items (GNOME_POCKET (object));

	g_main_loop_quit (loop);
}

static void
cached_cb (GObject *object,
	   GAsyncResult *res,
	   gpointer user_data)
{
	gboolean ret;

	ret = gnome_pocket_load_cached_finish (GNOME_POCKET (object), res, NULL);
	if (!ret) {
		g_message ("Failed to load the cached data");
		return;
	}

#if 0
	print_items (GNOME_POCKET (object));
	g_main_loop_quit (loop);
	return;
#endif
	g_message ("launching a refresh");
	gnome_pocket_refresh (GNOME_POCKET (object),
			      NULL,
			      refresh_cb,
			      NULL);
}

static void
is_available (GObject    *gobject,
	      GParamSpec *pspec,
	      gpointer    user_data)
{
	gboolean avail;

	g_object_get (user_data, "available", &avail, NULL);
	if (!avail) {
		return;
	}

	g_message ("Loading cached data");
	gnome_pocket_load_cached (GNOME_POCKET (user_data),
				  NULL,
				  cached_cb,
				  NULL);
}

int main (int argc, char **argv)
{
	GnomePocket *pocket;

	setlocale (LC_ALL, "");

	g_set_prgname ("org.gnome.Pocket.test");

	loop = g_main_loop_new (NULL, FALSE);
	pocket = gnome_pocket_new ();
	g_signal_connect (pocket, "notify::available",
			  G_CALLBACK (is_available), pocket);

	g_main_loop_run (loop);

	g_object_unref (pocket);

	return 0;
}
