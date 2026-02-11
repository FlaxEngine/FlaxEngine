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

#include <libportal/remote.h>
#include <libportal/inputcapture.h>

struct _XdpSession {
  GObject parent_instance;

  /* Generic Session implementation */
  XdpPortal *portal;
  char *id;
  gboolean is_closed;
  XdpSessionType type;
  guint signal_id;

  /* RemoteDesktop/ScreenCast */
  XdpSessionState state;
  XdpDeviceType devices;
  GVariant *streams;

  XdpPersistMode persist_mode;
  char *restore_token;

  gboolean uses_eis;

  /* InputCapture */
  XdpInputCaptureSession *input_capture_session; /* weak ref */
};

XdpSession * _xdp_session_new (XdpPortal *portal,
                               const char *id,
                               XdpSessionType type);

void         _xdp_session_set_session_state (XdpSession *session,
                                             XdpSessionState state);

void         _xdp_session_set_devices (XdpSession *session,
                                       XdpDeviceType devices);

void         _xdp_session_set_streams (XdpSession *session,
                                       GVariant   *streams);

void         _xdp_session_close (XdpSession *session);
