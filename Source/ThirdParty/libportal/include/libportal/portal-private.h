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

#include "glib-backports.h"
#include "parent-private.h"
#include "portal-helpers.h"

struct _XdpPortal {
  GObject parent_instance;

  GError *init_error;
  GDBusConnection *bus;
  char *sender;

  /* inhibit */
  int next_inhibit_id;
  GHashTable *inhibit_handles;
  char *session_monitor_handle;
  guint state_changed_signal;

  /* spawn */
  guint spawn_exited_signal;

  /* updates */
  char *update_monitor_handle;
  guint update_available_signal;
  guint update_progress_signal;

  /* location */
  char *location_monitor_handle;
  guint location_updated_signal;

  /* notification */
  guint action_invoked_signal;
  guint notification_interface_version;
  GVariant *supported_notification_options;

  /* screencast */
  guint screencast_interface_version;
  guint remote_desktop_interface_version;

  /* background */
  guint background_interface_version;
};

const char * portal_get_bus_name (void);

#define PORTAL_BUS_NAME (portal_get_bus_name ())
#define PORTAL_OBJECT_PATH  "/org/freedesktop/portal/desktop"
#define REQUEST_PATH_PREFIX "/org/freedesktop/portal/desktop/request/"
#define SESSION_PATH_PREFIX "/org/freedesktop/portal/desktop/session/"
#define REQUEST_INTERFACE "org.freedesktop.portal.Request"
#define SESSION_INTERFACE "org.freedesktop.portal.Session"
#define SETTINGS_INTERFACE "org.freedesktop.portal.Settings"

#define FLATPAK_PORTAL_BUS_NAME "org.freedesktop.portal.Flatpak"
#define FLATPAK_PORTAL_OBJECT_PATH "/org/freedesktop/portal/Flatpak"
#define FLATPAK_PORTAL_INTERFACE "org.freedesktop.portal.Flatpak"
