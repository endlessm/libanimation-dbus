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

#include <glib-object.h>

#include <animations-dbus-server-effect.h>
#include <animations-dbus-server-surface-attached-effect-interface.h>
#include <animations-dbus-server-types.h>

G_BEGIN_DECLS

#define ANIMATIONS_DBUS_TYPE_SERVER_SURFACE_BRIDGE animations_dbus_server_surface_bridge_get_type ()

struct _AnimationsDbusServerSurfaceBridgeInterface
{
  GTypeInterface parent_iface;

  AnimationsDbusServerSurfaceAttachedEffect * (*attach_effect) (AnimationsDbusServerSurfaceBridge  *self,
                                                                const char                         *event,
                                                                AnimationsDbusServerEffect         *effect,
                                                                GError                            **error);

  void (*detach_effect) (AnimationsDbusServerSurfaceBridge         *self,
                         const char                                *event,
                         AnimationsDbusServerSurfaceAttachedEffect *attached_effect);

  const char * (*get_title) (AnimationsDbusServerSurfaceBridge *self);

  GVariant * (*get_geometry) (AnimationsDbusServerSurfaceBridge *self);

  GVariant * (*get_available_effects) (AnimationsDbusServerSurfaceBridge *self);
};

AnimationsDbusServerSurfaceAttachedEffect * animations_dbus_server_surface_bridge_attach_effect (AnimationsDbusServerSurfaceBridge  *self,
                                                                                                 const char                         *event,
                                                                                                 AnimationsDbusServerEffect         *effect,
                                                                                                 GError                            **error);

void animations_dbus_server_surface_bridge_detach_effect (AnimationsDbusServerSurfaceBridge         *self,
                                                          const char                                *event,
                                                          AnimationsDbusServerSurfaceAttachedEffect *attached_effect);

const char * animations_dbus_server_surface_bridge_get_title (AnimationsDbusServerSurfaceBridge *self);

GVariant * animations_dbus_server_surface_bridge_get_geometry (AnimationsDbusServerSurfaceBridge *self);

GVariant * animations_dbus_server_surface_bridge_get_available_effects (AnimationsDbusServerSurfaceBridge *self);

G_END_DECLS
