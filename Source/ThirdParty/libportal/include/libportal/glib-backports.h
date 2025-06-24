/*
 * Copyright © 2024 GNOME Foundation Inc.
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

#include <errno.h>

#include <glib/gstdio.h>

#if !GLIB_CHECK_VERSION(2, 76, 0)

static inline gboolean
g_clear_fd (int     *fd_ptr,
            GError **error)
{
  int fd = *fd_ptr;

  *fd_ptr = -1;

  if (fd < 0)
    return TRUE;

  /* Suppress "Not available before" warning */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  return g_close (fd, error);
  G_GNUC_END_IGNORE_DEPRECATIONS
}

static inline void
_g_clear_fd_ignore_error (int *fd_ptr)
{
  /* Don't overwrite thread-local errno if closing the fd fails */
  int errsv = errno;

  /* Suppress "Not available before" warning */
  G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  if (!g_clear_fd (fd_ptr, NULL))
    {
      /* Do nothing: we ignore all errors, except for EBADF which
       * is a programming error, checked for by g_close(). */
    }

  G_GNUC_END_IGNORE_DEPRECATIONS

  errno = errsv;
}

#define g_autofd __attribute__((cleanup(_g_clear_fd_ignore_error)))

#endif

#if !GLIB_CHECK_VERSION(2, 84, 0)
static inline unsigned int
backport_steal_handle_id (unsigned int *handle_pointer)
{
  unsigned int handle;

  handle = *handle_pointer;
  *handle_pointer = 0;

  return handle;
}
#define g_steal_handle_id(hp) backport_steal_handle_id (hp)
#endif
