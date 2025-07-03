/*
 * Copyright (C) 2018, Matthias Clasen
 * Copyright (C) 2024 GNOME Foundation, Inc.
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
 *
 * Authors:
 *    Matthias Clasen
 *    Hubert Figuière <hub@figuiere.net>
 */

#pragma once

#include <gio/gio.h>

#include <libportal/types.h>

G_BEGIN_DECLS

#ifndef XDP_PUBLIC
#define XDP_PUBLIC extern
#endif

#define XDP_TYPE_PORTAL (xdp_portal_get_type ())

XDP_PUBLIC
G_DECLARE_FINAL_TYPE (XdpPortal, xdp_portal, XDP, PORTAL, GObject)

XDP_PUBLIC
XdpPortal *xdp_portal_new                   (void);

XDP_PUBLIC
XdpPortal *xdp_portal_initable_new          (GError **error);

XDP_PUBLIC
gboolean   xdp_portal_running_under_flatpak (void);

XDP_PUBLIC
gboolean   xdp_portal_running_under_snap    (GError **error);

XDP_PUBLIC
gboolean   xdp_portal_running_under_sandbox (void);

XDP_PUBLIC
XdpSettings *xdp_portal_get_settings        (XdpPortal *portal);

G_END_DECLS
