/*
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
 *    Hubert Figuière <hub@figuiere.net>
 */

#pragma once

#include <libportal/types.h>

G_BEGIN_DECLS

#define XDP_TYPE_SETTINGS (xdp_settings_get_type ())

XDP_PUBLIC
G_DECLARE_FINAL_TYPE (XdpSettings, xdp_settings, XDP, SETTINGS, GObject)

XDP_PUBLIC
GVariant *xdp_settings_read_value (XdpSettings *settings, const char *namespace, const char *key, GCancellable *cancellable, GError **error);

XDP_PUBLIC
void
xdp_settings_read (XdpSettings *settings, const char *namespace,
                   const gchar *key,
                   GCancellable *cancellable, GError **error,
                   const gchar *format, ...);

XDP_PUBLIC
guint xdp_settings_read_uint (XdpSettings *settings, const char *namespace, const char *key, GCancellable *cancellable, GError **error);

XDP_PUBLIC
char *xdp_settings_read_string (XdpSettings *settings, const char *namespace, const char *key, GCancellable *cancellable, GError **error);

XDP_PUBLIC
GVariant *xdp_settings_read_all_values (XdpSettings *settings, const char *const *namespaces, GCancellable *cancellable, GError **error);

G_END_DECLS
