/*
 * Copyright (C) 2021, Georges Basile Stavracas Neto
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

#define XDP_TYPE_PARENT (xdp_parent_get_type ())

XDP_PUBLIC
GType xdp_parent_get_type (void) G_GNUC_CONST;

XDP_PUBLIC
XdpParent *xdp_parent_copy (XdpParent *source);

XDP_PUBLIC
void xdp_parent_free (XdpParent *parent);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (XdpParent, xdp_parent_free)

G_END_DECLS
