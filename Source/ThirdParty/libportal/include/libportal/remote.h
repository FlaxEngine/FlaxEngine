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
#include <libportal/session.h>

G_BEGIN_DECLS

/**
 * XdpSessionState:
 * @XDP_SESSION_INITIAL: the session has not been started.
 * @XDP_SESSION_ACTIVE: the session is active.
 * @XDP_SESSION_CLOSED: the session is no longer active.
 *
 * The state of a session.
 */
typedef enum {
  XDP_SESSION_INITIAL,
  XDP_SESSION_ACTIVE,
  XDP_SESSION_CLOSED
} XdpSessionState;

/**
 * XdpOutputType:
 * @XDP_OUTPUT_NONE: do not select any output
 * @XDP_OUTPUT_MONITOR: allow selecting monitors
 * @XDP_OUTPUT_WINDOW: allow selecting individual application windows
 * @XDP_OUTPUT_VIRTUAL: allow creating new virtual displays
 *
 * Flags to specify what kind of sources to offer for a screencast session.
 */
typedef enum {
  XDP_OUTPUT_NONE = 0,
  XDP_OUTPUT_MONITOR = 1 << 0,
  XDP_OUTPUT_WINDOW  = 1 << 1,
  XDP_OUTPUT_VIRTUAL = 1 << 2,
} XdpOutputType;

/**
 * XdpDeviceType:
 * @XDP_DEVICE_NONE: no device
 * @XDP_DEVICE_KEYBOARD: control the keyboard.
 * @XDP_DEVICE_POINTER: control the pointer.
 * @XDP_DEVICE_TOUCHSCREEN: control the touchscreen.
 *
 * Flags to specify what input devices to control for a remote desktop session.
 */
typedef enum {
  XDP_DEVICE_NONE        = 0,
  XDP_DEVICE_KEYBOARD    = 1 << 0,
  XDP_DEVICE_POINTER     = 1 << 1,
  XDP_DEVICE_TOUCHSCREEN = 1 << 2
} XdpDeviceType;

/**
 * XdpScreencastFlags:
 * @XDP_SCREENCAST_FLAG_NONE: No options
 * @XDP_SCREENCAST_FLAG_MULTIPLE: allow opening multiple streams
 *
 * Options for starting screen casts.
 */
typedef enum {
  XDP_SCREENCAST_FLAG_NONE     = 0,
  XDP_SCREENCAST_FLAG_MULTIPLE = 1 << 0
} XdpScreencastFlags;

/**
 * XdpCursorMode:
 * @XDP_CURSOR_MODE_HIDDEN: no cursor
 * @XDP_CURSOR_MODE_EMBEDDED: cursor is embedded on the stream
 * @XDP_CURSOR_MODE_METADATA: cursor is sent as metadata of the stream
 *
 * Options for how the cursor is handled.
 */
typedef enum {
  XDP_CURSOR_MODE_HIDDEN   = 1 << 0,
  XDP_CURSOR_MODE_EMBEDDED = 1 << 1,
  XDP_CURSOR_MODE_METADATA = 1 << 2,
} XdpCursorMode;

/**
 * XdpPersistMode:
 * @XDP_PERSIST_MODE_NONE: do not persist
 * @XDP_PERSIST_MODE_TRANSIENT: persist as long as the application is alive
 * @XDP_PERSIST_MODE_PERSISTENT: persist until the user revokes this permission
 *
 * Options for how the screencast session should persist.
 */
typedef enum {
  XDP_PERSIST_MODE_NONE,
  XDP_PERSIST_MODE_TRANSIENT,
  XDP_PERSIST_MODE_PERSISTENT,
} XdpPersistMode;

XDP_PUBLIC
void        xdp_portal_create_screencast_session            (XdpPortal            *portal,
                                                             XdpOutputType         outputs,
                                                             XdpScreencastFlags    flags,
                                                             XdpCursorMode         cursor_mode,
                                                             XdpPersistMode        persist_mode,
                                                             const char           *restore_token,
                                                             GCancellable         *cancellable,
                                                             GAsyncReadyCallback   callback,
                                                             gpointer              data);

XDP_PUBLIC
XdpSession *xdp_portal_create_screencast_session_finish     (XdpPortal            *portal,
                                                             GAsyncResult         *result,
                                                             GError              **error);

/**
 * XdpRemoteDesktopFlags:
 * @XDP_REMOTE_DESKTOP_FLAG_NONE: No options
 * @XDP_REMOTE_DESKTOP_FLAG_MULTIPLE: allow opening multiple streams
 *
 * Options for starting remote desktop sessions.
 */
typedef enum {
  XDP_REMOTE_DESKTOP_FLAG_NONE     = 0,
  XDP_REMOTE_DESKTOP_FLAG_MULTIPLE = 1 << 0
} XdpRemoteDesktopFlags;

XDP_PUBLIC
void        xdp_portal_create_remote_desktop_session        (XdpPortal              *portal,
                                                             XdpDeviceType           devices,
                                                             XdpOutputType           outputs,
                                                             XdpRemoteDesktopFlags   flags,
                                                             XdpCursorMode           cursor_mode,
                                                             GCancellable           *cancellable,
                                                             GAsyncReadyCallback     callback,
                                                             gpointer                data);

XDP_PUBLIC
void        xdp_portal_create_remote_desktop_session_full   (XdpPortal              *portal,
                                                             XdpDeviceType           devices,
                                                             XdpOutputType           outputs,
                                                             XdpRemoteDesktopFlags   flags,
                                                             XdpCursorMode           cursor_mode,
                                                             XdpPersistMode          persist_mode,
                                                             const char             *restore_token,
                                                             GCancellable           *cancellable,
                                                             GAsyncReadyCallback     callback,
                                                             gpointer                data);


XDP_PUBLIC
XdpSession *xdp_portal_create_remote_desktop_session_finish (XdpPortal              *portal,
                                                             GAsyncResult           *result,
                                                             GError                **error);

XDP_PUBLIC
XdpSessionState xdp_session_get_session_state (XdpSession *session);

XDP_PUBLIC
void        xdp_session_start                (XdpSession           *session,
                                              XdpParent            *parent,
                                              GCancellable         *cancellable,
                                              GAsyncReadyCallback   callback,
                                              gpointer              data);

XDP_PUBLIC
gboolean    xdp_session_start_finish         (XdpSession           *session,
                                              GAsyncResult         *result,
                                              GError              **error);

XDP_PUBLIC
int         xdp_session_open_pipewire_remote (XdpSession           *session);

XDP_PUBLIC
XdpDeviceType   xdp_session_get_devices       (XdpSession *session);

XDP_PUBLIC
GVariant *      xdp_session_get_streams       (XdpSession *session);

XDP_PUBLIC
int       xdp_session_connect_to_eis    (XdpSession  *session,
                                         GError     **error);

XDP_PUBLIC
void      xdp_session_pointer_motion    (XdpSession *session,
                                         double      dx,
                                         double      dy);

XDP_PUBLIC
void      xdp_session_pointer_position  (XdpSession *session,
                                         guint       stream,
                                         double      x,
                                         double      y);
/**
 * XdpButtonState:
 * @XDP_BUTTON_RELEASED: the button is down
 * @XDP_BUTTON_PRESSED: the button is up
 *
 * The XdpButtonState enumeration is used to describe
 * the state of buttons.
 */
typedef enum {
  XDP_BUTTON_RELEASED = 0,
  XDP_BUTTON_PRESSED = 1
} XdpButtonState;

XDP_PUBLIC
void      xdp_session_pointer_button (XdpSession     *session,
                                      int             button,
                                      XdpButtonState  state);

XDP_PUBLIC
void      xdp_session_pointer_axis   (XdpSession     *session,
                                      gboolean        finish,
                                      double          dx,
                                      double          dy);

/**
 * XdpDiscreteAxis:
 * @XDP_AXIS_HORIZONTAL_SCROLL: the horizontal scroll axis
 * @XDP_AXIS_VERTICAL_SCROLL: the horizontal scroll axis
 *
 * The `XdpDiscreteAxis` enumeration is used to describe
 * the discrete scroll axes.
 */
typedef enum {
  XDP_AXIS_HORIZONTAL_SCROLL = 0,
  XDP_AXIS_VERTICAL_SCROLL   = 1
} XdpDiscreteAxis;

XDP_PUBLIC
void      xdp_session_pointer_axis_discrete (XdpSession *session,
                                             XdpDiscreteAxis  axis,
                                             int              steps);

/**
 * XdpKeyState:
 * @XDP_KEY_RELEASED: the key is down
 * @XDP_KEY_PRESSED: the key is up
 *
 * The `XdpKeyState` enumeration is used to describe
 * the state of keys.
 */
typedef enum {
  XDP_KEY_RELEASED = 0,
  XDP_KEY_PRESSED = 1
} XdpKeyState;

XDP_PUBLIC
void      xdp_session_keyboard_key   (XdpSession *session,
                                      gboolean    keysym,
                                      int         key,
                                      XdpKeyState state);

XDP_PUBLIC
void      xdp_session_touch_down     (XdpSession *session,
                                      guint       stream,
                                      guint       slot,
                                      double      x,
                                      double      y);

XDP_PUBLIC
void      xdp_session_touch_position (XdpSession *session,
                                      guint       stream,
                                      guint       slot,
                                      double      x,
                                      double      y);

XDP_PUBLIC
void      xdp_session_touch_up       (XdpSession *session,
                                      guint       slot);


XDP_PUBLIC
XdpPersistMode  xdp_session_get_persist_mode  (XdpSession *session);

XDP_PUBLIC
char           *xdp_session_get_restore_token (XdpSession *session);

G_END_DECLS
