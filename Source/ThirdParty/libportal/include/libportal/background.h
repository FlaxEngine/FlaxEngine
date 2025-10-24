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
 * XdpBackgroundFlags:
 * @XDP_BACKGROUND_FLAG_NONE: No options
 * @XDP_BACKGROUND_FLAG_AUTOSTART: Request autostart as well
 * @XDP_BACKGROUND_FLAG_ACTIVATABLE: Whether the application is D-Bus-activatable
 *
 * Options to use when requesting background.
 */
typedef enum {
  XDP_BACKGROUND_FLAG_NONE        = 0,
  XDP_BACKGROUND_FLAG_AUTOSTART   = 1 << 0,
  XDP_BACKGROUND_FLAG_ACTIVATABLE = 1 << 1
} XdpBackgroundFlags;

XDP_PUBLIC
void      xdp_portal_request_background         (XdpPortal           *portal,
                                                 XdpParent           *parent,
                                                 char                *reason,
                                                 GPtrArray           *commandline,
                                                 XdpBackgroundFlags   flags,
                                                 GCancellable        *cancellable,
                                                 GAsyncReadyCallback  callback,
                                                 gpointer             user_data);

XDP_PUBLIC
gboolean   xdp_portal_request_background_finish (XdpPortal           *portal,
                                                 GAsyncResult        *result,
                                                 GError             **error);

XDP_PUBLIC
void        xdp_portal_set_background_status         (XdpPortal            *portal,
                                                      const char           *status_message,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              data);

XDP_PUBLIC
gboolean    xdp_portal_set_background_status_finish  (XdpPortal            *portal,
                                                      GAsyncResult         *result,
                                                      GError              **error);

G_END_DECLS
