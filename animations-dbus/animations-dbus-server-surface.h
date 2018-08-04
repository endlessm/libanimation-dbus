/* Copyright 2018 Endless Mobile, Inc.
 *
 * libanimation-dbus is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * libanimation-dbus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with eos-discovery-feed.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 * - Sam Spilsbury <sam@endlessm.com>
 */

#pragma once

#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>

#include <animations-dbus-objects.h>
#include <animations-dbus-server-surface-bridge-interface.h>
#include <animations-dbus-server-types.h>

G_BEGIN_DECLS

#define ANIMATIONS_DBUS_TYPE_SERVER_SURFACE animations_dbus_server_surface_get_type ()


gboolean animations_dbus_server_surface_export (AnimationsDbusServerSurface  *server_surface,
                                                const char                   *object_path,
                                                GError                      **error);

void animations_dbus_server_surface_unexport (AnimationsDbusServerSurface *server_surface);

gboolean animations_dbus_server_surface_attach_animation_effect_with_server_priority (AnimationsDbusServerSurface  *server_surface,
                                                                                      const char                   *event,
                                                                                      AnimationsDbusServerEffect   *server_animation_effect,
                                                                                      GError                      **error);

AnimationsDbusServerSurfaceAttachedEffect * animations_dbus_server_surface_highest_priority_attached_effect_for_event (AnimationsDbusServerSurface *server_surface,
                                                                                                                       const char                  *event);

void animations_dbus_server_surface_emit_geometry_changed (AnimationsDbusServerSurface *server_surface);

void animations_dbus_server_surface_emit_title_changed (AnimationsDbusServerSurface *server_surface);

AnimationsDbusServerSurface * animations_dbus_server_surface_new (GDBusConnection                   *connection,
                                                                  AnimationsDbusServer              *server,
                                                                  AnimationsDbusServerSurfaceBridge *bridge);

G_END_DECLS
