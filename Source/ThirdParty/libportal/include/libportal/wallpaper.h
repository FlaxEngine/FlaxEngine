/*
 * Copyright (C) 2019, Matthias Clasen
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 3.0 of the
 * License.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#pragma once

#include <libportal/types.h>

G_BEGIN_DECLS

/**
 * XdpWallpaperFlags:
 * @XDP_WALLPAPER_FLAG_NONE: No flags
 * @XDP_WALLPAPER_FLAG_BACKGROUND: Set wallpaper on the desktop background
 * @XDP_WALLPAPER_FLAG_LOCKSCREEN: Set wallpaper on the lockscreen
 * @XDP_WALLPAPER_FLAG_PREVIEW: Request the preview to be shown
 *
 * The values of this enumeration determine where the wallpaper is being set.
 */
typedef enum {
  XDP_WALLPAPER_FLAG_NONE       = 0,  
  XDP_WALLPAPER_FLAG_BACKGROUND = 1 << 0,
  XDP_WALLPAPER_FLAG_LOCKSCREEN = 1 << 1,
  XDP_WALLPAPER_FLAG_PREVIEW    = 1 << 2
} XdpWallpaperFlags;

#define XDP_WALLPAPER_TARGET_BOTH (XDP_WALLPAPER_TARGET_BACKGROUND|XDP_WALLPAPER_TARGET_LOCKSCREEN)

XDP_PUBLIC
void       xdp_portal_set_wallpaper           (XdpPortal            *portal,
                                               XdpParent            *parent,
                                               const char           *uri,
                                               XdpWallpaperFlags     flags,
                                               GCancellable         *cancellable,
                                               GAsyncReadyCallback   callback,
                                               gpointer              data);

XDP_PUBLIC
gboolean  xdp_portal_set_wallpaper_finish     (XdpPortal            *portal,
                                               GAsyncResult         *result,
                                               GError              **error);

G_END_DECLS
