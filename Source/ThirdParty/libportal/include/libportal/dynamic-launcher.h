/*
 * Copyright (C) 2022, Matthew Leeds
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
 * XdpLauncherType:
 * @XDP_LAUNCHER_APPLICATION: a launcher for a regular application
 * @XDP_LAUNCHER_WEBAPP: a launcher for a web app
 *
 * The type of a launcher.
 */
typedef enum {
  XDP_LAUNCHER_APPLICATION = 1 << 0,
  XDP_LAUNCHER_WEBAPP      = 1 << 1
} XdpLauncherType;

XDP_PUBLIC
void      xdp_portal_dynamic_launcher_prepare_install        (XdpPortal           *portal,
                                                              XdpParent           *parent,
                                                              const char          *name,
                                                              GVariant            *icon_v,
                                                              XdpLauncherType      launcher_type,
                                                              const char          *target,
                                                              gboolean             editable_name,
                                                              gboolean             editable_icon,
                                                              GCancellable        *cancellable,
                                                              GAsyncReadyCallback  callback,
                                                              gpointer             data);

XDP_PUBLIC
GVariant *xdp_portal_dynamic_launcher_prepare_install_finish (XdpPortal            *portal,
                                                              GAsyncResult         *result,
                                                              GError              **error);

XDP_PUBLIC
char     *xdp_portal_dynamic_launcher_request_install_token  (XdpPortal   *portal,
                                                              const char  *name,
                                                              GVariant    *icon_v,
                                                              GError     **error);

XDP_PUBLIC
gboolean  xdp_portal_dynamic_launcher_install                (XdpPortal   *portal,
                                                              const char  *token,
                                                              const char  *desktop_file_id,
                                                              const char  *desktop_entry,
                                                              GError     **error);

XDP_PUBLIC
gboolean  xdp_portal_dynamic_launcher_uninstall              (XdpPortal   *portal,
                                                              const char  *desktop_file_id,
                                                              GError     **error);

XDP_PUBLIC
char     *xdp_portal_dynamic_launcher_get_desktop_entry      (XdpPortal   *portal,
                                                              const char  *desktop_file_id,
                                                              GError     **error);

XDP_PUBLIC
GVariant *xdp_portal_dynamic_launcher_get_icon               (XdpPortal   *portal,
                                                              const char  *desktop_file_id,
                                                              char       **out_icon_format,
                                                              guint       *out_icon_size,
                                                              GError     **error);

XDP_PUBLIC
gboolean  xdp_portal_dynamic_launcher_launch                 (XdpPortal   *portal,
                                                              const char  *desktop_file_id,
                                                              const char  *activation_token,
                                                              GError     **error);

G_END_DECLS
