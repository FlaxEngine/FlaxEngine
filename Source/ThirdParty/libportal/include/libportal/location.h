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
 * XdpLocationAccuracy:
 * @XDP_LOCATION_ACCURACY_NONE: No particular accuracy
 * @XDP_LOCATION_ACCURACY_COUNTRY: Country-level accuracy
 * @XDP_LOCATION_ACCURACY_CITY: City-level accuracy
 * @XDP_LOCATION_ACCURACY_NEIGHBORHOOD: Neighborhood-level accuracy
 * @XDP_LOCATION_ACCURACY_STREET: Street-level accuracy
 * @XDP_LOCATION_ACCURACY_EXACT: Maximum accuracy
 *
 * The values of this enum indicate the desired level
 * of accuracy for location information.
 */
typedef enum {
  XDP_LOCATION_ACCURACY_NONE,
  XDP_LOCATION_ACCURACY_COUNTRY,
  XDP_LOCATION_ACCURACY_CITY,
  XDP_LOCATION_ACCURACY_NEIGHBORHOOD,
  XDP_LOCATION_ACCURACY_STREET,
  XDP_LOCATION_ACCURACY_EXACT
} XdpLocationAccuracy;

typedef enum {
  XDP_LOCATION_MONITOR_FLAG_NONE = 0
} XdpLocationMonitorFlags;

XDP_PUBLIC
void     xdp_portal_location_monitor_start        (XdpPortal                *portal,
                                                   XdpParent                *parent,
                                                   guint                     distance_threshold,
                                                   guint                     time_threshold,
                                                   XdpLocationAccuracy       accuracy,
                                                   XdpLocationMonitorFlags   flags,
                                                   GCancellable             *cancellable,
                                                   GAsyncReadyCallback       callback,
                                                   gpointer                  data);

XDP_PUBLIC
gboolean xdp_portal_location_monitor_start_finish (XdpPortal                *portal,
                                                   GAsyncResult             *result,
                                                   GError                  **error);

XDP_PUBLIC
void     xdp_portal_location_monitor_stop         (XdpPortal                *portal);

G_END_DECLS
