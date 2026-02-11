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
 * XdpSpawnFlags:
 * @XDP_SPAWN_FLAG_NONE: No flags
 * @XDP_SPAWN_FLAG_CLEARENV: Clear the environment
 * @XDP_SPAWN_FLAG_LATEST: Spawn the latest version of the app
 * @XDP_SPAWN_FLAG_SANDBOX: Spawn in a sandbox (equivalent to the --sandbox option of flatpak run)
 * @XDP_SPAWN_FLAG_NO_NETWORK: Spawn without network (equivalent to the --unshare=network option of flatpak run)
 * @XDP_SPAWN_FLAG_WATCH: Kill the sandbox when the caller disappears from the session bus
 *
 * Flags influencing the spawn operation and how the
 * new sandbox is created.
 */
typedef enum {
  XDP_SPAWN_FLAG_NONE       = 0,
  XDP_SPAWN_FLAG_CLEARENV   = 1 << 0,
  XDP_SPAWN_FLAG_LATEST     = 1 << 1,
  XDP_SPAWN_FLAG_SANDBOX    = 1 << 2,
  XDP_SPAWN_FLAG_NO_NETWORK = 1 << 3,
  XDP_SPAWN_FLAG_WATCH      = 1 << 4,
} XdpSpawnFlags;

XDP_PUBLIC
void         xdp_portal_spawn                 (XdpPortal            *portal,
                                               const char           *cwd,
                                               const char * const   *argv,
                                               int                  *fds,
                                               int                  *map_to,
                                               int                   n_fds,
                                               const char * const   *env,
                                               XdpSpawnFlags         flags,
                                               const char * const   *sandbox_expose,
                                               const char * const   *sandbox_expose_ro,
                                               GCancellable         *cancellable,
                                               GAsyncReadyCallback   callback,
                                               gpointer              data);

XDP_PUBLIC
pid_t        xdp_portal_spawn_finish          (XdpPortal            *portal,
                                               GAsyncResult         *result,
                                               GError              **error);

XDP_PUBLIC
void        xdp_portal_spawn_signal           (XdpPortal            *portal,
                                               pid_t                 pid,
                                               int                   signal,
                                               gboolean              to_process_group);

G_END_DECLS
