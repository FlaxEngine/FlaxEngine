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
 * XdpUpdateStatus:
 * @XDP_UPDATE_STATUS_RUNNING: Installation in progress
 * @XDP_UPDATE_STATUS_EMPTY: Nothing to install
 * @XDP_UPDATE_STATUS_DONE: Installation finished successfully
 * @XDP_UPDATE_STATUS_FAILED: Installation failed
 *
 * The values of this enum are returned in the
 * [signal@Portal::update-progress] signal to indicate
 * the current progress of an installation.
 */
typedef enum {
  XDP_UPDATE_STATUS_RUNNING,
  XDP_UPDATE_STATUS_EMPTY,
  XDP_UPDATE_STATUS_DONE,
  XDP_UPDATE_STATUS_FAILED
} XdpUpdateStatus;

typedef enum
{
  XDP_UPDATE_MONITOR_FLAG_NONE = 0,
} XdpUpdateMonitorFlags;

XDP_PUBLIC
void       xdp_portal_update_monitor_start        (XdpPortal              *portal,
                                                   XdpUpdateMonitorFlags   flags,
                                                   GCancellable           *cancellable,
                                                   GAsyncReadyCallback     callback,
                                                   gpointer                data);

XDP_PUBLIC
gboolean   xdp_portal_update_monitor_start_finish (XdpPortal              *portal,
                                                   GAsyncResult           *result,
                                                   GError                **error);

XDP_PUBLIC
void       xdp_portal_update_monitor_stop         (XdpPortal              *portal);

typedef enum
{
  XDP_UPDATE_INSTALL_FLAG_NONE = 0,
} XdpUpdateInstallFlags;

XDP_PUBLIC
void       xdp_portal_update_install              (XdpPortal              *portal,
                                                   XdpParent              *parent,
                                                   XdpUpdateInstallFlags   flags,
                                                   GCancellable           *cancellable,
                                                   GAsyncReadyCallback     callback,
                                                   gpointer                data);

XDP_PUBLIC
gboolean   xdp_portal_update_install_finish       (XdpPortal              *portal,
                                                   GAsyncResult           *result,
                                                   GError                **error);

G_END_DECLS
