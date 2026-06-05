/*
 * Copyright (C) 2022, Red Hat, Inc.
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

#include "inputcapture-pointerbarrier.h"
#include "inputcapture-zone.h"

guint
_xdp_input_capture_pointer_barrier_get_id (XdpInputCapturePointerBarrier *barrier);

void
_xdp_input_capture_pointer_barrier_set_is_active (XdpInputCapturePointerBarrier *barrier, gboolean active);

void
_xdp_input_capture_zone_invalidate_and_free  (XdpInputCaptureZone *zone);
