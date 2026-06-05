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

#include <libportal/portal.h>
#include <gtk/gtk.h>

#if !(GTK_CHECK_VERSION(3,96,0) || GTK_CHECK_VERSION(4,0,0))
#error "To use libportal with GTK3, include portal-gtk3.h"
#endif

G_BEGIN_DECLS

XDP_PUBLIC
XdpParent *xdp_parent_new_gtk (GtkWindow *window);

G_END_DECLS
