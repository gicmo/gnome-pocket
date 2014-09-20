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
 */

#ifndef GNOME_POCKET_CLIENT_H
#define GNOME_POCKET_CLIENT_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define GNOME_TYPE_POCKET_CLIENT         (gnome_pocket_client_get_type ())
#define GNOME_POCKET_CLIENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_TYPE_POCKET_CLIENT, GnomePocketClient))
#define GNOME_POCKET_CLIENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GNOME_TYPE_POCKET_CLIENT, GnomePocketClientClass))
#define GNOME_IS_POCKET(o)               (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_TYPE_POCKET_CLIENT))
#define GNOME_IS_POCKET_CLIENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_TYPE_POCKET_CLIENT))
#define GNOME_POCKET_CLIENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GNOME_TYPE_POCKET_CLIENT, GnomePocketClientClass))

typedef struct _GnomePocketClient        GnomePocketClient;
typedef struct _GnomePocketClientClass   GnomePocketClientClass;
typedef struct _GnomePocketClientPrivate GnomePocketClientPrivate;

struct _GnomePocketClient
{
  /*< private >*/
  GObject parent;
  GnomePocketClientPrivate *priv;
};

struct _GnomePocketClientClass
{
  /*< private >*/
  GObjectClass parent;
};

GType                gnome_pocket_client_get_type        (void) G_GNUC_CONST;

GnomePocketClient   *gnome_pocket_client_new             (void);
void                 gnome_pocket_client_add_url         (GnomePocketClient         *self,
                                                          const char                *url,
                                                          const char                *tweet_id,
                                                          GCancellable              *cancellable,
                                                          GAsyncReadyCallback        callback,
                                                          gpointer                   user_data);
gboolean             gnome_pocket_client_add_url_finish  (GnomePocketClient         *self,
                                                          GAsyncResult              *res,
                                                          GError                   **error);
void                 gnome_pocket_client_refresh         (GnomePocketClient         *self,
                                                          GCancellable              *cancellable,
                                                          GAsyncReadyCallback        callback,
                                                          gpointer                   user_data);
gboolean             gnome_pocket_client_refresh_finish  (GnomePocketClient         *self,
                                                          GAsyncResult              *res,
                                                          GError                   **error);
GList               *gnome_pocket_client_get_items       (GnomePocketClient         *self);

void                 gnome_pocket_client_load_cached        (GnomePocketClient       *self,
                                                             GCancellable            *cancellable,
                                                             GAsyncReadyCallback      callback,
                                                             gpointer                 user_data);
gboolean             gnome_pocket_client_load_cached_finish (GnomePocketClient       *self,
                                                             GAsyncResult            *res,
                                                             GError                 **error);

G_END_DECLS

#endif /* GNOME_POCKET_CLIENT_H */
