/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 *  Copyright © 2013 Bastien Nocera <hadess@hadess.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any pocket version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Implementation of:
 * http://getpocket.com/developer/docs/overview
 */

//#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#define GOA_API_IS_SUBJECT_TO_CHANGE 1
#include <goa/goa.h>
#include <rest/rest-proxy.h>
#include <rest/rest-proxy-call.h>
#include <json-glib/json-glib.h>

#include "gnome-pocket.h"
#include "gnome-pocket-item.h"

enum {
  PROP_0,
  PROP_AVAILABLE
};

G_DEFINE_TYPE (GnomePocket, gnome_pocket, G_TYPE_OBJECT);

#define GNOME_POCKET_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GNOME_TYPE_POCKET, GnomePocketPrivate))

struct _GnomePocketPrivate {
  GCancellable   *cancellable;
  GoaClient      *client;
  GoaOAuth2Based *oauth2;
  char           *access_token;
  char           *consumer_key;
  RestProxy      *proxy;

  /* List data */
  GomRepository *repository;
  gboolean       cache_loaded;
  gint64         since;
  GList         *items; /* GnomePocketItem */
};

gboolean
gnome_pocket_refresh_finish (GnomePocket       *self,
                             GAsyncResult        *res,
                             GError             **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
  gboolean ret = FALSE;

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == gnome_pocket_refresh);

  if (!g_simple_async_result_propagate_error (simple, error))
    ret = g_simple_async_result_get_op_res_gboolean (simple);

  return ret;
}

/* FIXME */
static char *cache_path = NULL;

static char *
get_string_for_element (JsonReader *reader,
                        const char *element)
{
  char *ret;

  if (!json_reader_read_member (reader, element)) {
    json_reader_end_member (reader);
    return NULL;
  }
  ret = g_strdup (json_reader_get_string_value (reader));
  if (ret && *ret == '\0')
    g_clear_pointer (&ret, g_free);
  json_reader_end_member (reader);

  return ret;
}

static int
get_int_for_element (JsonReader *reader,
                     const char *element)
{
  int ret;

  if (!json_reader_read_member (reader, element)) {
    json_reader_end_member (reader);
    return -1;
  }
  ret = atoi (json_reader_get_string_value (reader));
  json_reader_end_member (reader);

  return ret;
}

static gint64
get_time_added (JsonReader *reader)
{
  gint64 ret;

  if (!json_reader_read_member (reader, "time_added")) {
    json_reader_end_member (reader);
    return -1;
  }
  ret = g_ascii_strtoll (json_reader_get_string_value (reader), NULL, 0);
  json_reader_end_member (reader);

  return ret;
}

static GnomePocketItem *
parse_item (JsonReader    *reader,
	    GomRepository *repository)
{
  GObject *item;
  char *str;
  GnomePocketItemStatus status;
  gboolean favorite, is_article;
  GnomePocketMediaInclusion has_image, has_video;
  gint64 time_added;
  char **tags;

  str = g_strdup (json_reader_get_member_name (reader));
  if (!str)
    goto bail;

  item = g_object_new (GNOME_TYPE_POCKET_ITEM,
		       "repository", repository,
		       "id", str,
		       NULL);
  g_clear_pointer (&str, g_free);

  /* If the item is archived or deleted, we don't need
   * anything more here */
  status = get_int_for_element (reader, "status");
  g_object_set (item, "status", status, NULL);
  if (status != GNOME_POCKET_STATUS_NORMAL)
    goto end;

  str = get_string_for_element (reader, "resolved_url");
  if (!str)
    str = get_string_for_element (reader, "given_url");
  g_object_set (item, "url", str, NULL);
  g_clear_pointer (&str, g_free);

  str = get_string_for_element (reader, "resolved_title");
  if (!str)
    str = get_string_for_element (reader, "given_title");
  if (!str)
    str = g_strdup ("PLACEHOLDER"); /* FIXME generate from URL */
  g_object_set (item, "title", str, NULL);
  g_clear_pointer (&str, g_free);

  favorite = get_int_for_element (reader, "favorite");
  is_article = get_int_for_element (reader, "is_article");
  if (is_article == -1)
    is_article = FALSE;
  has_image = get_int_for_element (reader, "has_image");
  if (has_image == -1)
    has_image = GNOME_POCKET_HAS_MEDIA_FALSE;
  has_video = get_int_for_element (reader, "has_video");
  if (has_video == -1)
    has_video = GNOME_POCKET_HAS_MEDIA_FALSE;
  g_object_set (item,
		"favorite", favorite,
		"is-article", is_article,
		"has-image", has_image,
		"has-video", has_video,
		NULL);

  time_added = get_time_added (reader);
  g_object_set (item, "time-added", time_added, NULL);

  if (json_reader_read_member (reader, "tags")) {
    tags = json_reader_list_members (reader);
    g_object_set (item, "tags", tags, NULL);
    g_clear_pointer (&tags, g_strfreev);
  }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "image")) {
    str = get_string_for_element (reader, "src");
    g_object_set (item, "thumbnail-url", str, NULL);
    g_free (str);
  }

  json_reader_end_member (reader);

  goto end;

bail:
  g_clear_object (&item);

end:
  return GNOME_POCKET_ITEM (item);
}

static void
update_list (GnomePocket *self,
             GList       *updated_items)
{
  GHashTable *removed; /* key=id, value=gboolean */
  GList *added;
  GList *l;

  if (updated_items == NULL)
    return;

  removed = g_hash_table_new_full (g_str_hash, g_str_equal,
                                   g_free, NULL);

  added = NULL;
  for (l = updated_items; l != NULL; l = l->next) {
    GnomePocketItem *item = l->data;
    GnomePocketItemStatus status;
    char *id;

    g_object_get (G_OBJECT (item),
		  "status", &status,
		  "id", &id, NULL);

    if (status != GNOME_POCKET_STATUS_NORMAL) {
      g_hash_table_insert (removed,
                           id,
                           GINT_TO_POINTER (1));
      g_object_unref (item);
    } else {
      GError *error = NULL;

      added = g_list_prepend (added, item);
      if (!gom_resource_save_sync (GOM_RESOURCE (item), &error)) {
	g_warning ("Failed to save item '%s': %s", id, error->message);
	g_error_free (error);
      }
      g_free (id);
    }
  }

  added = g_list_reverse (added);
  self->priv->items = g_list_concat (added, self->priv->items);

  /* And remove the old items */
  for (l = self->priv->items; l != NULL; l = l->next) {
    GnomePocketItem *item = l->data;
    char *id;

    g_object_get (G_OBJECT (item), "id", &id, NULL);

    if (g_hash_table_lookup (removed, id)) {
      GError *error = NULL;

      /* Item got removed */
      self->priv->items = g_list_delete_link (self->priv->items, l);

      if (!gom_resource_delete_sync (GOM_RESOURCE (item), &error)) {
	g_warning ("Failed to remove item '%s': %s", id, error->message);
	g_error_free (error);
      }
      g_object_unref (item);
    }
    g_free (id);
  }

  g_hash_table_destroy (removed);
}

static gint64
load_since (GnomePocket *self)
{
  char *path;
  char *contents = NULL;
  gint64 since = 0;

  path = g_build_filename (cache_path, "since", NULL);
  g_file_get_contents (path, &contents, NULL, NULL);
  g_free (path);

  if (contents != NULL) {
    since = g_ascii_strtoll (contents, NULL, 0);
    g_free (contents);
  }

  return since;
}

static void
save_since (GnomePocket *self)
{
  char *str;
  char *path;

  if (self->priv->since == 0)
    return;

  str = g_strdup_printf ("%" G_GINT64_FORMAT, self->priv->since);
  path = g_build_filename (cache_path, "since", NULL);
  g_file_set_contents (path, str, -1, NULL);
  g_free (path);
  g_free (str);
}

static int
sort_items (gconstpointer a,
	    gconstpointer b)
{
  GnomePocketItem *item_a = (gpointer) a;
  GnomePocketItem *item_b = (gpointer) b;
  gint64 time_added_a, time_added_b;

  g_object_get (G_OBJECT (item_a), "time-added", &time_added_a, NULL);
  g_object_get (G_OBJECT (item_b), "time-added", &time_added_b, NULL);

  /* We sort newest first */
  if (time_added_a < time_added_b)
    return 1;
  if (time_added_b < time_added_a)
    return -1;
  return 0;
}

static GList *
parse_json (JsonParser    *parser,
            gint64        *since,
	    GomRepository *repository)
{
  JsonReader *reader;
  GList *ret;
  int num;

  reader = json_reader_new (json_parser_get_root (parser));
  *since = 0;
  ret = NULL;

  num = json_reader_count_members (reader);
  if (num < 0)
    goto bail;

  /* Grab the since */
  if (json_reader_read_member (reader, "since"))
    *since = json_reader_get_int_value (reader);
  json_reader_end_member (reader);

  /* Grab the list */
  if (json_reader_read_member (reader, "list")) {
    char **members;
    guint i;

    members = json_reader_list_members (reader);
    if (members != NULL) {
      for (i = 0; members[i] != NULL; i++) {
        GnomePocketItem *item;

        if (!json_reader_read_member (reader, members[i])) {
          json_reader_end_member (reader);
          continue;
        }
        item = parse_item (reader, repository);
        if (item)
          ret = g_list_prepend (ret, item);
        json_reader_end_member (reader);
      }
    }
    g_strfreev (members);
  }
  json_reader_end_member (reader);

  ret = g_list_sort (ret, sort_items);

bail:
  g_clear_object (&reader);
  return ret;
}

static void
refresh_cb (GObject      *object,
            GAsyncResult *res,
            gpointer      user_data)
{
  GError *error = NULL;
  GSimpleAsyncResult *simple = user_data;
  gboolean ret;

  ret = rest_proxy_call_invoke_finish (REST_PROXY_CALL (object), res, &error);
  if (!ret) {
    g_simple_async_result_set_from_error (simple, error);
  } else {
    JsonParser *parser;

    parser = json_parser_new ();
    if (json_parser_load_from_data (parser,
                                    rest_proxy_call_get_payload (REST_PROXY_CALL (object)),
                                    rest_proxy_call_get_payload_length (REST_PROXY_CALL (object)),
                                    NULL)) {
      GList *updated_items;
      GnomePocket *self;

      self = GNOME_POCKET (g_async_result_get_source_object (G_ASYNC_RESULT (simple)));
      updated_items = parse_json (parser, &self->priv->since, self->priv->repository);
      if (self->priv->since != 0)
        save_since (self);
      update_list (self, updated_items);
      g_object_unref (self);
    }
    g_object_unref (parser);
  }
  g_simple_async_result_set_op_res_gboolean (simple, ret);

  g_simple_async_result_complete_in_idle (simple);
  g_object_unref (simple);
  g_clear_error (&error);
}

void
gnome_pocket_refresh (GnomePocket         *self,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  RestProxyCall *call;
  GSimpleAsyncResult *simple;

  g_return_if_fail (GNOME_IS_POCKET (self));
  g_return_if_fail (self->priv->consumer_key && self->priv->access_token);
  g_return_if_fail (GOM_IS_REPOSITORY (self->priv->repository));

  simple = g_simple_async_result_new (G_OBJECT (self),
                                      callback,
                                      user_data,
                                      gnome_pocket_refresh);

  g_simple_async_result_set_check_cancellable (simple, cancellable);

  call = rest_proxy_new_call (self->priv->proxy);
  rest_proxy_call_set_method (call, "POST");
  rest_proxy_call_set_function (call, "v3/get");
  rest_proxy_call_add_param (call, "consumer_key", self->priv->consumer_key);
  rest_proxy_call_add_param (call, "access_token", self->priv->access_token);

  if (self->priv->since > 0) {
    char *since;
    since = g_strdup_printf ("%" G_GINT64_FORMAT, self->priv->since);
    rest_proxy_call_add_param (call, "since", since);
    g_free (since);
  }

  /* To get the image/images/authors/videos item details */
  rest_proxy_call_add_param (call, "detailType", "complete");
  rest_proxy_call_add_param (call, "tags", "1");

  rest_proxy_call_invoke_async (call, cancellable, refresh_cb, simple);
  g_object_unref (call);
}

gboolean
gnome_pocket_add_url_finish (GnomePocket   *self,
                             GAsyncResult  *res,
                             GError       **error)
{
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
  gboolean ret = FALSE;

  g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == gnome_pocket_add_url);

  if (!g_simple_async_result_propagate_error (simple, error))
    ret = g_simple_async_result_get_op_res_gboolean (simple);

  return ret;
}

static void
add_url_cb (GObject      *object,
            GAsyncResult *res,
            gpointer      user_data)
{
  GError *error = NULL;
  GSimpleAsyncResult *simple = user_data;
  gboolean ret;

  ret = rest_proxy_call_invoke_finish (REST_PROXY_CALL (object), res, &error);
  if (!ret)
    g_simple_async_result_set_from_error (simple, error);
  g_simple_async_result_set_op_res_gboolean (simple, ret);

  g_simple_async_result_complete_in_idle (simple);
  g_object_unref (simple);
  g_clear_error (&error);
}

void
gnome_pocket_add_url (GnomePocket         *self,
                      const char          *url,
                      const char          *tweet_id,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  RestProxyCall *call;
  GSimpleAsyncResult *simple;

  g_return_if_fail (GNOME_IS_POCKET (self));
  g_return_if_fail (url);
  g_return_if_fail (self->priv->consumer_key && self->priv->access_token);

  simple = g_simple_async_result_new (G_OBJECT (self),
                                      callback,
                                      user_data,
                                      gnome_pocket_add_url);

  g_simple_async_result_set_check_cancellable (simple, cancellable);

  call = rest_proxy_new_call (self->priv->proxy);
  rest_proxy_call_set_method (call, "POST");
  rest_proxy_call_set_function (call, "v3/add");
  rest_proxy_call_add_param (call, "consumer_key", self->priv->consumer_key);
  rest_proxy_call_add_param (call, "access_token", self->priv->access_token);
  rest_proxy_call_add_param (call, "url", url);
  if (tweet_id)
    rest_proxy_call_add_param (call, "tweet_id", tweet_id);

  rest_proxy_call_invoke_async (call, cancellable, add_url_cb, simple);
}

static void
load_cached_thread (GTask           *task,
                    gpointer         source_object,
                    gpointer         task_data,
                    GCancellable    *cancellable)
{
  GnomePocket *self = GNOME_POCKET (source_object);
  GomResourceGroup *group;
  GError *error = NULL;
  guint count, i;

  self->priv->since = load_since (self);

  group = gom_repository_find_sync (self->priv->repository,
				    GNOME_TYPE_POCKET_ITEM,
				    NULL,
				    &error);
  if (!group) {
    g_warning ("Failed to fetch items from the database: %s", error->message);
    g_task_return_error (task, error);
    return;
  }

  count = gom_resource_group_get_count (group);
  if (!gom_resource_group_fetch_sync (group, 0, count, &error)) {
    g_warning ("Failed to fetch items from the database: %s", error->message);
    g_object_unref (group);
    g_task_return_error (task, error);
    return;
  }

  for (i = 0; i < count; i++) {
    GnomePocketItem *item;

    item = GNOME_POCKET_ITEM (gom_resource_group_get_index (group, i));
    self->priv->items = g_list_prepend (self->priv->items, g_object_ref(item));
  }

  g_object_unref (group);
  self->priv->items = g_list_sort (self->priv->items, sort_items);

  self->priv->cache_loaded = TRUE;
  g_task_return_boolean (task, TRUE);
}

void
gnome_pocket_load_cached (GnomePocket         *self,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data)
{
  GTask *task;

  g_return_if_fail (GNOME_IS_POCKET (self));
  g_return_if_fail (!self->priv->cache_loaded);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_run_in_thread (task, load_cached_thread);
  g_object_unref (task);
}

gboolean
gnome_pocket_load_cached_finish (GnomePocket         *self,
                                 GAsyncResult        *res,
                                 GError             **error)
{
  GTask *task = G_TASK (res);

  g_return_val_if_fail (g_task_is_valid (res, self), NULL);

  return g_task_propagate_boolean (task, error);
}

/**
 * gnome_pocket_get_items:
 * @self: a #GnomePocket
 *
 * Gets the items that Pocket knows about.
 *
 * Returns: (element-type GnomePocketItem) (transfer none): A list of
 * items or %NULL. Do not modify or free the list, it belongs to the
 * #GnomePocket object.
 **/
GList *
gnome_pocket_get_items (GnomePocket *self)
{
  g_return_val_if_fail (self->priv->cache_loaded, NULL);
  return self->priv->items;
}

static void
gnome_pocket_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
  GnomePocket *self = GNOME_POCKET (object);

  switch (property_id) {
  case PROP_AVAILABLE:
    g_value_set_boolean (value, self->priv->access_token && self->priv->consumer_key);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
    break;
  }
}

static void
gnome_pocket_finalize (GObject *object)
{
  GnomePocketPrivate *priv = GNOME_POCKET (object)->priv;
  GError *error = NULL;

  g_cancellable_cancel (priv->cancellable);
  g_clear_object (&priv->cancellable);

  g_clear_object (&priv->proxy);
  g_clear_object (&priv->oauth2);
  g_clear_object (&priv->client);
  g_clear_pointer (&priv->access_token, g_free);
  g_clear_pointer (&priv->consumer_key, g_free);

  g_list_free_full (priv->items, g_object_unref);

  if (priv->repository) {
    GomAdapter *adapter;

    adapter = gom_repository_get_adapter (priv->repository);
    if (!gom_adapter_close_sync (adapter, &error)) {
      g_warning ("Failed to close the DB adapter: %s", error->message);
      g_error_free (error);
    }
    g_clear_object (&priv->repository);
  }

  G_OBJECT_CLASS (gnome_pocket_parent_class)->finalize (object);
}

static void
gnome_pocket_class_init (GnomePocketClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_assert (g_get_prgname ());

  cache_path = g_build_filename (g_get_user_cache_dir (), g_get_prgname (), NULL);
  g_mkdir_with_parents (cache_path, 0700);

  object_class->get_property = gnome_pocket_get_property;
  object_class->finalize = gnome_pocket_finalize;

  g_object_class_install_property (object_class,
                                   PROP_AVAILABLE,
                                   g_param_spec_boolean ("available",
                                                         "Available",
                                                         "If Read Pocket is available",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));

  g_type_class_add_private (object_class, sizeof (GnomePocketPrivate));
}

static void
migrate_cb (GObject      *object,
	    GAsyncResult *result,
	    gpointer      user_data)
{
  GnomePocket *self = user_data;
  GomRepository *repository = (GomRepository *)object;
  GError *error = NULL;

  g_object_set_data (object, "object-types", NULL);

  if (!gom_repository_migrate_finish (repository, result, &error)) {
    g_warning ("Failed to migrate database to latest version: %s", error->message);
    g_error_free (error);
    return;
  }

  g_object_notify (G_OBJECT (self), "available");
}

static void
open_cb (GObject      *object,
	 GAsyncResult *result,
	 gpointer      user_data)
{
  GnomePocket *self = user_data;
  GomAdapter *adapter = (GomAdapter *)object;
  GError *error = NULL;
  GList *object_types;

  if (!gom_adapter_open_finish (adapter, result, &error)) {
    g_warning ("Failed to open database: %s", error->message);
    return;
  }

  self->priv->repository = gom_repository_new (adapter);
  object_types = g_list_prepend (NULL, GINT_TO_POINTER(GNOME_TYPE_POCKET_ITEM));
  g_object_set_data_full (G_OBJECT (self->priv->repository), "object-types", object_types, (GDestroyNotify) g_list_free);
  gom_repository_automatic_migrate_async (self->priv->repository, 1, object_types, migrate_cb, user_data);
}

static char *
get_db_uri (void)
{
  char *path, *uri;

  path = g_build_filename (cache_path, "pocket.db", NULL);
  uri = g_filename_to_uri (path, NULL, NULL);
  g_free (path);
  return uri;
}

static void
setup_database (GnomePocket *self)
{
  GomAdapter *adapter;
  char *uri;

  adapter = gom_adapter_new ();
  uri = get_db_uri ();
  gom_adapter_open_async (adapter, uri, open_cb, self);
  g_free (uri);
  g_object_unref (adapter);
}

static void
got_access_token (GObject       *object,
                  GAsyncResult  *res,
                  GnomePocket   *self)
{
  GError *error = NULL;
  char *access_token;

  if (!goa_oauth2_based_call_get_access_token_finish (GOA_OAUTH2_BASED (object),
                                                      &access_token,
                                                      NULL,
                                                      res,
                                                      &error)) {
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_error_free (error);
      return;
    }
    g_warning ("Failed to get access token: %s", error->message);
    g_error_free (error);
    return;
  }

  self->priv->access_token = access_token;
  self->priv->consumer_key = goa_oauth2_based_dup_client_id (GOA_OAUTH2_BASED (object));

  /* Network side all setup, now onto the database */
  setup_database (self);
}

static void
handle_accounts (GnomePocket *self)
{
  GList *accounts, *l;
  GoaOAuth2Based *oauth2 = NULL;

  g_clear_object (&self->priv->oauth2);
  g_clear_pointer (&self->priv->access_token, g_free);
  g_clear_pointer (&self->priv->consumer_key, g_free);

  accounts = goa_client_get_accounts (self->priv->client);

  for (l = accounts; l != NULL; l = l->next) {
    GoaObject *object = GOA_OBJECT (l->data);
    GoaAccount *account;

    account = goa_object_peek_account (object);

    /* Find a Pocket account that doesn't have "Read Pocket" disabled */
    if (g_strcmp0 (goa_account_get_provider_type (account), "pocket") == 0 &&
        !goa_account_get_read_later_disabled (account)) {
      oauth2 = goa_object_get_oauth2_based (object);
      break;
    }
  }

  g_list_free_full (accounts, (GDestroyNotify) g_object_unref);

  if (!oauth2) {
    g_object_notify (G_OBJECT (self), "available");
    g_debug ("Could not find a Pocket account");
    return;
  }

  self->priv->oauth2 = oauth2;

  goa_oauth2_based_call_get_access_token (oauth2,
                                          self->priv->cancellable,
                                          (GAsyncReadyCallback) got_access_token,
                                          self);
}

static void
account_added_cb (GoaClient     *client,
                  GoaObject     *object,
                  GnomePocket *self)
{
  if (self->priv->oauth2 != NULL) {
    /* Don't care, already have an account */
    return;
  }

  handle_accounts (self);
}

static void
account_changed_cb (GoaClient     *client,
                    GoaObject     *object,
                    GnomePocket *self)
{
  GoaOAuth2Based *oauth2;

  oauth2 = goa_object_get_oauth2_based (object);
  if (oauth2 == self->priv->oauth2)
    handle_accounts (self);

  g_object_unref (oauth2);
}

static void
account_removed_cb (GoaClient     *client,
                    GoaObject     *object,
                    GnomePocket *self)
{
  GoaOAuth2Based *oauth2;

  oauth2 = goa_object_get_oauth2_based (object);
  if (oauth2 == self->priv->oauth2)
    handle_accounts (self);

  g_object_unref (oauth2);
}

static void
client_ready_cb (GObject       *source_object,
                 GAsyncResult  *res,
                 GnomePocket *self)
{
  GoaClient *client;
  GError *error = NULL;

  client = goa_client_new_finish (res, &error);
  if (client == NULL) {
    if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_error_free (error);
      return;
    }
    g_warning ("Failed to get GoaClient: %s", error->message);
    g_error_free (error);
    return;
  }

  self->priv->client = client;
  g_signal_connect (self->priv->client, "account-added",
                    G_CALLBACK (account_added_cb), self);
  g_signal_connect (self->priv->client, "account-changed",
                    G_CALLBACK (account_changed_cb), self);
  g_signal_connect (self->priv->client, "account-removed",
                    G_CALLBACK (account_removed_cb), self);

  handle_accounts (self);
}

static void
gnome_pocket_init (GnomePocket *self)
{
  self->priv = GNOME_POCKET_GET_PRIVATE (self);
  self->priv->cancellable = g_cancellable_new ();
  self->priv->proxy = rest_proxy_new ("https://getpocket.com/", FALSE);

  goa_client_new (self->priv->cancellable,
                  (GAsyncReadyCallback) client_ready_cb, self);
}

/**
 * gnome_pocket_new:
 *
 * Creates a new #GnomePocket object.
 *
 * Returns: (transfer full): a new #GnomePocket object. Use g_object_unref() when done.
 **/
GnomePocket *
gnome_pocket_new (void)
{
  return g_object_new (GNOME_TYPE_POCKET, NULL);
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
