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

#ifndef GNOME_POCKET_H
#define GNOME_POCKET_H

#include <gio/gio.h>

G_BEGIN_DECLS

#define GNOME_TYPE_POCKET         (gnome_pocket_get_type ())
#define GNOME_POCKET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_TYPE_POCKET, GnomePocket))
#define GNOME_POCKET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GNOME_TYPE_POCKET, GnomePocketClass))
#define GNOME_IS_POCKET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GNOME_TYPE_POCKET))
#define GNOME_IS_POCKET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GNOME_TYPE_POCKET))
#define GNOME_POCKET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GNOME_TYPE_POCKET, GnomePocketClass))

typedef struct _GnomePocket        GnomePocket;
typedef struct _GnomePocketClass   GnomePocketClass;
typedef struct _GnomePocketPrivate GnomePocketPrivate;

struct _GnomePocket
{
  /*< private >*/
  GObject parent;
  GnomePocketPrivate *priv;
};

struct _GnomePocketClass
{
  /*< private >*/
  GObjectClass parent;
};

GType              gnome_pocket_get_type        (void);

GnomePocket       *gnome_pocket_new             (void);
void               gnome_pocket_add_url         (GnomePocket         *self,
                                                 const char          *url,
                                                 const char          *tweet_id,
                                                 GCancellable        *cancellable,
                                                 GAsyncReadyCallback  callback,
                                                 gpointer             user_data);
gboolean           gnome_pocket_add_url_finish  (GnomePocket         *self,
                                                 GAsyncResult        *res,
                                                 GError             **error);
void               gnome_pocket_refresh         (GnomePocket         *self,
                                                 GCancellable        *cancellable,
                                                 GAsyncReadyCallback  callback,
                                                 gpointer             user_data);
gboolean           gnome_pocket_refresh_finish  (GnomePocket         *self,
                                                 GAsyncResult        *res,
                                                 GError             **error);
GList             *gnome_pocket_get_items       (GnomePocket         *self);

void               gnome_pocket_load_cached        (GnomePocket         *self,
                                                    GCancellable        *cancellable,
                                                    GAsyncReadyCallback  callback,
                                                    gpointer             user_data);
gboolean           gnome_pocket_load_cached_finish (GnomePocket         *self,
                                                    GAsyncResult        *res,
                                                    GError             **error);

G_END_DECLS

#endif /* GNOME_POCKET_H */
