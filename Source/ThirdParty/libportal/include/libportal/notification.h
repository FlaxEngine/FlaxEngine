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

typedef enum {
  XDP_NOTIFICATION_FLAG_NONE = 0
} XdpNotificationFlags;

XDP_PUBLIC
void       xdp_portal_add_notification        (XdpPortal             *portal,
                                               const char            *id,
                                               GVariant              *notification,
                                               XdpNotificationFlags   flags,
                                               GCancellable          *cancellable,
                                               GAsyncReadyCallback    callback,
                                               gpointer               data);

XDP_PUBLIC
gboolean   xdp_portal_add_notification_finish (XdpPortal             *portal,
                                               GAsyncResult          *result,
                                               GError               **error);

XDP_PUBLIC
void       xdp_portal_remove_notification     (XdpPortal             *portal,
                                               const char            *id);

XDP_PUBLIC
GVariant  *xdp_portal_get_supported_notification_options (XdpPortal  *portal,
                                                          GError    **error);

G_END_DECLS
