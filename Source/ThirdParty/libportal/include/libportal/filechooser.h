/*
 * Copyright (C) 2018, Matthias Clasen
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
 * XdpOpenFileFlags:
 * @XDP_OPEN_FILE_FLAG_NONE: No options
 * @XDP_OPEN_FILE_FLAG_MULTIPLE: Allow selecting multiple files
 *
 * Options for opening files.
 */
typedef enum {
  XDP_OPEN_FILE_FLAG_NONE     = 0,
  XDP_OPEN_FILE_FLAG_MULTIPLE = 1 << 0,
} XdpOpenFileFlags;

XDP_PUBLIC
void       xdp_portal_open_file                   (XdpPortal            *portal,
                                                   XdpParent            *parent,
                                                   const char           *title,
                                                   GVariant             *filters,
                                                   GVariant             *current_filter,
                                                   GVariant             *choices,
                                                   XdpOpenFileFlags      flags,
                                                   GCancellable         *cancellable,
                                                   GAsyncReadyCallback   callback,
                                                   gpointer              data);

XDP_PUBLIC
GVariant *xdp_portal_open_file_finish             (XdpPortal            *portal,
                                                   GAsyncResult         *result,
                                                   GError              **error);

typedef enum {
  XDP_SAVE_FILE_FLAG_NONE = 0
} XdpSaveFileFlags;

XDP_PUBLIC
void       xdp_portal_save_file                   (XdpPortal            *portal,
                                                   XdpParent            *parent,
                                                   const char           *title,
                                                   const char           *current_name,
                                                   const char           *current_folder,
                                                   const char           *current_file,
                                                   GVariant             *filters,
                                                   GVariant             *current_filter,
                                                   GVariant             *choices,
                                                   XdpSaveFileFlags      flags,
                                                   GCancellable         *cancellable,
                                                   GAsyncReadyCallback   callback,
                                                   gpointer              data);

XDP_PUBLIC
GVariant *xdp_portal_save_file_finish             (XdpPortal            *portal,
                                                   GAsyncResult         *result,
                                                   GError              **error);

XDP_PUBLIC
void       xdp_portal_save_files                  (XdpPortal            *portal,
                                                   XdpParent            *parent,
                                                   const char           *title,
                                                   const char           *current_name,
                                                   const char           *current_folder,
                                                   GVariant             *files,
                                                   GVariant             *choices,
                                                   XdpSaveFileFlags      flags,
                                                   GCancellable         *cancellable,
                                                   GAsyncReadyCallback   callback,
                                                   gpointer              data);

XDP_PUBLIC
GVariant *xdp_portal_save_files_finish            (XdpPortal            *portal,
                                                   GAsyncResult         *result,
                                                   GError              **error);

G_END_DECLS
