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

#include "gnome-pocket-item.h"

struct _GnomePocketItemPrivate {
  char                  *id;
  char                  *url;
  char                  *title;
  char                  *thumbnail_url;
  gboolean               favorite;
  PocketItemStatus       status;
  gboolean               is_article;
  PocketMediaInclusion   has_image;
  PocketMediaInclusion   has_video;
  gint64                 time_added;
  char                 **tags;
};

G_DEFINE_TYPE(GnomePocketItem, gnome_pocket_item, GOM_TYPE_RESOURCE)

#define GNOME_POCKET_ITEM_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GNOME_TYPE_POCKET_ITEM, GnomePocketItemPrivate))

enum {
  PROP_0,
  PROP_ID,
  PROP_URL,
  PROP_TITLE,
  PROP_THUMBNAIL_URL,
  PROP_FAVORITE,
  PROP_STATUS,
  PROP_IS_ARTICLE,
  PROP_HAS_IMAGE,
  PROP_HAS_VIDEO,
  PROP_TIME_ADDED,
  PROP_TAGS
};

static void
gnome_pocket_item_finalize (GObject *object)
{
  GnomePocketItem *item = (GnomePocketItem *) (object);

  g_free (item->priv->id);
  g_free (item->priv->url);
  g_free (item->priv->title);
  g_strfreev (item->priv->tags);
}

static const char *
bool_to_str (gboolean b)
{
  return b ? "True" : "False";
}

static const char *
inclusion_to_str (PocketMediaInclusion inc)
{
  switch (inc) {
  case POCKET_HAS_MEDIA_FALSE:
    return "False";
  case POCKET_HAS_MEDIA_INCLUDED:
    return "Included";
  case POCKET_IS_MEDIA:
    return "Is media";
  default:
    g_assert_not_reached ();
  }
}

void
gnome_pocket_item_print (GnomePocketItem *item)
{
  GDateTime *date;
  char *date_str;

  g_return_if_fail (item != NULL);

  date = g_date_time_new_from_unix_utc (item->priv->time_added);
  date_str = g_date_time_format (date, "%F %R");
  g_date_time_unref (date);

  g_print ("Item: %s\n", item->priv->id);
  g_print ("\tTime added: %s\n", date_str);
  g_print ("\tURL: %s\n", item->priv->url);
  if (item->priv->thumbnail_url)
    g_print ("\tThumbnail URL: %s\n", item->priv->thumbnail_url);
  g_print ("\tTitle: %s\n", item->priv->title);
  g_print ("\tFavorite: %s\n", bool_to_str (item->priv->favorite));
  g_print ("\tIs article: %s\n", bool_to_str (item->priv->is_article));
  g_print ("\tHas Image: %s\n", inclusion_to_str (item->priv->has_image));
  g_print ("\tHas Video: %s\n", inclusion_to_str (item->priv->has_video));
  if (item->priv->tags != NULL) {
    guint i;
    g_print ("\tTags: ");
    for (i = 0; item->priv->tags[i] != NULL; i++)
      g_print ("%s, ", item->priv->tags[i]);
    g_print ("\n");
  }

  g_free (date_str);
}

static void
gnome_pocket_item_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
  GnomePocketItem *self = GNOME_POCKET_ITEM (object);

  switch (property_id) {
  case PROP_ID:
    g_value_set_string (value, self->priv->id);
    break;
  case PROP_URL:
    g_value_set_string (value, self->priv->url);
    break;
  case PROP_TITLE:
    g_value_set_string (value, self->priv->title);
    break;
  case PROP_THUMBNAIL_URL:
    g_value_set_string (value, self->priv->thumbnail_url);
    break;
  case PROP_FAVORITE:
    g_value_set_boolean (value, self->priv->favorite);
    break;
  case PROP_STATUS:
    g_value_set_uint (value, self->priv->status);
    break;
  case PROP_IS_ARTICLE:
    g_value_set_boolean (value, self->priv->is_article);
    break;
  case PROP_HAS_IMAGE:
    g_value_set_uint (value, self->priv->has_image);
    break;
  case PROP_HAS_VIDEO:
    g_value_set_uint (value, self->priv->has_video);
    break;
  case PROP_TIME_ADDED:
    g_value_set_int64 (value, self->priv->time_added);
    break;
  case PROP_TAGS:
    g_value_set_boxed (value, self->priv->tags);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
    break;
  }
}

static void
gnome_pocket_item_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
  GnomePocketItem *self = GNOME_POCKET_ITEM (object);

  switch (property_id) {
  case PROP_ID:
    g_free (self->priv->id);
    self->priv->id = g_value_dup_string (value);
    break;
  case PROP_URL:
    g_free (self->priv->url);
    self->priv->url = g_value_dup_string (value);
    break;
  case PROP_TITLE:
    g_free (self->priv->title);
    self->priv->title = g_value_dup_string (value);
    break;
  case PROP_THUMBNAIL_URL:
    g_free (self->priv->thumbnail_url);
    self->priv->thumbnail_url = g_value_dup_string (value);
    break;
  case PROP_FAVORITE:
    self->priv->favorite = g_value_get_boolean (value);
    break;
  case PROP_STATUS:
    self->priv->status = g_value_get_uint (value);
    break;
  case PROP_IS_ARTICLE:
    self->priv->is_article = g_value_get_boolean (value);
    break;
  case PROP_HAS_IMAGE:
    self->priv->has_image = g_value_get_uint (value);
    break;
  case PROP_HAS_VIDEO:
    self->priv->has_video = g_value_get_uint (value);
    break;
  case PROP_TIME_ADDED:
    self->priv->time_added = g_value_get_int64 (value);
    break;
  case PROP_TAGS:
    if (self->priv->tags)
      g_strfreev (self->priv->tags);
    self->priv->tags = g_value_dup_boxed (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
    break;
  }
}

static void
gnome_pocket_item_class_init (GnomePocketItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GomResourceClass *resource_class = GOM_RESOURCE_CLASS (klass);

  g_type_class_add_private (klass, sizeof(GnomePocketItemPrivate));

  object_class->get_property = gnome_pocket_item_get_property;
  object_class->set_property = gnome_pocket_item_set_property;
  object_class->finalize = gnome_pocket_item_finalize;

  gom_resource_class_set_table(resource_class, "items");

#define STR_PROP(name, id) g_object_class_install_property(object_class, id, g_param_spec_string(name, name, name, NULL, G_PARAM_READWRITE));

  STR_PROP("id", PROP_ID);
  STR_PROP("url", PROP_URL);
  STR_PROP("title", PROP_TITLE);
  STR_PROP("thumbnail-url", PROP_THUMBNAIL_URL);
#undef STR_PROP

#define BOOL_PROP(name, id) g_object_class_install_property(object_class, id, g_param_spec_boolean(name, name, name, FALSE, G_PARAM_READWRITE));
  BOOL_PROP("favorite", PROP_FAVORITE);

  g_object_class_install_property (object_class,
                                   PROP_STATUS,
                                   g_param_spec_uint ("status",
                                                      "Status",
                                                      NULL,
                                                      POCKET_STATUS_NORMAL, POCKET_STATUS_DELETED, POCKET_STATUS_NORMAL,
						      G_PARAM_READWRITE));

  BOOL_PROP ("is-article", PROP_IS_ARTICLE);
#undef BOOL_PROP

  g_object_class_install_property (object_class,
                                   PROP_HAS_IMAGE,
                                   g_param_spec_uint ("has-image",
                                                      "Has image",
                                                      NULL,
                                                      POCKET_HAS_MEDIA_FALSE, POCKET_IS_MEDIA, POCKET_HAS_MEDIA_FALSE,
						      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_HAS_VIDEO,
                                   g_param_spec_uint ("has-video",
                                                      "Has video",
                                                      NULL,
                                                      POCKET_HAS_MEDIA_FALSE, POCKET_IS_MEDIA, POCKET_HAS_MEDIA_FALSE,
						      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_TIME_ADDED,
                                   g_param_spec_int64 ("time-added",
                                                       "Time added",
                                                       NULL,
						       G_MININT64, G_MAXINT64, 0,
						       G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   PROP_TAGS,
                                   g_param_spec_boxed ("tags",
                                                       "Tags",
                                                       NULL,
						       G_TYPE_STRV,
						       G_PARAM_READWRITE));

  gom_resource_class_set_primary_key(resource_class, "id");
}

static void
gnome_pocket_item_init (GnomePocketItem *item)
{
  item->priv = GNOME_POCKET_ITEM_GET_PRIVATE(item);
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
