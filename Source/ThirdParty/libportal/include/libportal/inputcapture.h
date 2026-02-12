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

#include <libportal/portal-helpers.h>
#include <libportal/session.h>
#include <libportal/inputcapture-zone.h>
#include <libportal/inputcapture-pointerbarrier.h>
#include <stdint.h>

G_BEGIN_DECLS

#define XDP_TYPE_INPUT_CAPTURE_SESSION (xdp_input_capture_session_get_type ())

XDP_PUBLIC
G_DECLARE_FINAL_TYPE (XdpInputCaptureSession, xdp_input_capture_session, XDP, INPUT_CAPTURE_SESSION, GObject)

/**
 * XdpInputCapability:
 * @XDP_INPUT_CAPABILITY_NONE: no device
 * @XDP_INPUT_CAPABILITY_KEYBOARD: capture the keyboard
 * @XDP_INPUT_CAPABILITY_POINTER: capture pointer events
 * @XDP_INPUT_CAPABILITY_TOUCHSCREEN: capture touchscreen events
 *
 * Flags to specify what input device capabilities should be captured
 */
typedef enum {
  XDP_INPUT_CAPABILITY_NONE         = 0,
  XDP_INPUT_CAPABILITY_KEYBOARD     = 1 << 0,
  XDP_INPUT_CAPABILITY_POINTER      = 1 << 1,
  XDP_INPUT_CAPABILITY_TOUCHSCREEN  = 1 << 2
} XdpInputCapability;


XDP_PUBLIC
void        xdp_portal_create_input_capture_session (XdpPortal            *portal,
                                                     XdpParent            *parent,
                                                     XdpInputCapability    capabilities,
                                                     GCancellable         *cancellable,
                                                     GAsyncReadyCallback   callback,
                                                     gpointer              data);

XDP_PUBLIC
XdpInputCaptureSession * xdp_portal_create_input_capture_session_finish (XdpPortal     *portal,
                                                                         GAsyncResult  *result,
                                                                         GError       **error);

XDP_PUBLIC
XdpSession  *xdp_input_capture_session_get_session (XdpInputCaptureSession *session);

XDP_PUBLIC
GList *     xdp_input_capture_session_get_zones (XdpInputCaptureSession *session);

XDP_PUBLIC
void        xdp_input_capture_session_set_pointer_barriers (XdpInputCaptureSession         *session,
                                                            GList                          *barriers,
                                                            GCancellable                   *cancellable,
                                                            GAsyncReadyCallback             callback,
                                                            gpointer                        data);

XDP_PUBLIC
GList *     xdp_input_capture_session_set_pointer_barriers_finish (XdpInputCaptureSession  *session,
                                                                   GAsyncResult            *result,
                                                                   GError                 **error);

XDP_PUBLIC
void        xdp_input_capture_session_enable (XdpInputCaptureSession *session);

XDP_PUBLIC
void        xdp_input_capture_session_disable (XdpInputCaptureSession *session);

XDP_PUBLIC
void        xdp_input_capture_session_release_at (XdpInputCaptureSession *session,
                                                  guint                   activation_id,
                                                  gdouble                 cursor_x_position,
                                                  gdouble                 cursor_y_position);

XDP_PUBLIC
void        xdp_input_capture_session_release (XdpInputCaptureSession *session,
                                               guint activation_id);

XDP_PUBLIC
int        xdp_input_capture_session_connect_to_eis (XdpInputCaptureSession  *session,
                                                     GError                 **error);

G_END_DECLS
