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
 * XdpInhibitFlags:
 * @XDP_INHIBIT_FLAG_LOGOUT: Inhibit logout
 * @XDP_INHIBIT_FLAG_USER_SWITCH: Inhibit user switching
 * @XDP_INHIBIT_FLAG_SUSPEND: Inhibit suspend
 * @XDP_INHIBIT_FLAG_IDLE: Inhibit the session going idle
 *
 * Flags that determine what session status changes are inhibited.
 */
typedef enum {
  XDP_INHIBIT_FLAG_LOGOUT      = 1 << 0,
  XDP_INHIBIT_FLAG_USER_SWITCH = 1 << 1,
  XDP_INHIBIT_FLAG_SUSPEND     = 1 << 2,
  XDP_INHIBIT_FLAG_IDLE        = 1 << 3
} XdpInhibitFlags;

XDP_PUBLIC
void       xdp_portal_session_inhibit             (XdpPortal            *portal,
                                                   XdpParent            *parent,
                                                   const char           *reason,
                                                   XdpInhibitFlags       flags,
                                                   GCancellable         *cancellable,
                                                   GAsyncReadyCallback   callback,
                                                   gpointer              data);

XDP_PUBLIC
int        xdp_portal_session_inhibit_finish      (XdpPortal            *portal,
                                                   GAsyncResult         *result,
                                                   GError              **error);

XDP_PUBLIC
void       xdp_portal_session_uninhibit           (XdpPortal            *portal,
                                                   int                   id);

/**
 * XdpLoginSessionState:
 * @XDP_LOGIN_SESSION_RUNNING: the session is running
 * @XDP_LOGIN_SESSION_QUERY_END: the session is in the query end phase,
 *     during which applications can save their state or inhibit the
 *     session from ending
 * @XDP_LOGIN_SESSION_ENDING: the session is about to end
 *
 * The values of this enum are returned in the [signal@Portal::session-state-changed] signal
 * to indicate the current state of the user session.
 */
typedef enum {
  XDP_LOGIN_SESSION_RUNNING =   1,
  XDP_LOGIN_SESSION_QUERY_END = 2,
  XDP_LOGIN_SESSION_ENDING =    3,
} XdpLoginSessionState;

typedef enum {
  XDP_SESSION_MONITOR_FLAG_NONE = 0
} XdpSessionMonitorFlags;

XDP_PUBLIC
void       xdp_portal_session_monitor_start              (XdpPortal               *portal,
                                                          XdpParent               *parent,
                                                          XdpSessionMonitorFlags   flags,
                                                          GCancellable            *cancellable,
                                                          GAsyncReadyCallback      callback,
                                                          gpointer                 data);

XDP_PUBLIC
gboolean   xdp_portal_session_monitor_start_finish       (XdpPortal               *portal,
                                                          GAsyncResult            *result,
                                                          GError                 **error);

XDP_PUBLIC
void       xdp_portal_session_monitor_stop               (XdpPortal               *portal);

XDP_PUBLIC
void       xdp_portal_session_monitor_query_end_response (XdpPortal               *portal);

G_END_DECLS
