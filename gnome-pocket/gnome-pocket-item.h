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

#ifndef GNOME_POCKET_ITEM_H
#define GNOME_POCKET_ITEM_H

#include <gio/gio.h>
#include <gom/gom.h>

G_BEGIN_DECLS

#define GNOME_POCKET_FAVORITE TRUE
#define GNOME_POCKET_NOT_FAVORITE FALSE

typedef enum {
  GNOME_POCKET_STATUS_NORMAL    = 0,
  GNOME_POCKET_STATUS_ARCHIVED  = 1,
  GNOME_POCKET_STATUS_DELETED   = 2
} GnomePocketItemStatus;

#define GNOME_POCKET_IS_ARTICLE TRUE
#define GNOME_POCKET_IS_NOT_ARTICLE FALSE

typedef enum {
  GNOME_POCKET_HAS_MEDIA_FALSE     = 0,
  GNOME_POCKET_HAS_MEDIA_INCLUDED  = 1,
  GNOME_POCKET_IS_MEDIA            = 2
} GnomePocketMediaInclusion;

typedef struct _GnomePocketItemPrivate GnomePocketItemPrivate;

typedef struct
{
  GomResourceClass parent;
} GnomePocketItemClass;

typedef struct {
  GomResource             parent;

  /*< private >*/
  GnomePocketItemPrivate *priv;
} GnomePocketItem;

#define GNOME_TYPE_POCKET_ITEM (gnome_pocket_item_get_type ())
#define GNOME_POCKET_ITEM(o)   (G_TYPE_CHECK_INSTANCE_CAST ((o), GNOME_TYPE_POCKET_ITEM, GnomePocketItem))

GType            gnome_pocket_item_get_type    (void) G_GNUC_CONST;
void             gnome_pocket_item_print       (GnomePocketItem *item);

G_END_DECLS

#endif /* GNOME_POCKET_ITEM_H */
