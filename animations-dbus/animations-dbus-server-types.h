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

G_BEGIN_DECLS

#include <glib-object.h>

#include <animations-dbus-objects.h>

G_DECLARE_FINAL_TYPE (AnimationsDbusServer, animations_dbus_server, ANIMATIONS_DBUS, SERVER, GObject)

G_DECLARE_INTERFACE (AnimationsDbusServerEffectFactory, animations_dbus_server_effect_factory, ANIMATIONS_DBUS, SERVER_EFFECT_FACTORY, GObject)

G_DECLARE_FINAL_TYPE (AnimationsDbusServerAnimationManager, animations_dbus_server_animation_manager, ANIMATIONS_DBUS, SERVER_ANIMATION_MANAGER, AnimationsDbusAnimationManagerSkeleton)

G_DECLARE_FINAL_TYPE (AnimationsDbusServerSurface, animations_dbus_server_surface, ANIMATIONS_DBUS, SERVER_SURFACE, AnimationsDbusAnimatableSurfaceSkeleton)

G_DECLARE_INTERFACE (AnimationsDbusServerSurfaceBridge, animations_dbus_server_surface_bridge, ANIMATIONS_DBUS, SERVER_SURFACE_BRIDGE, GObject)

G_DECLARE_FINAL_TYPE (AnimationsDbusServerEffect, animations_dbus_server_effect, ANIMATIONS_DBUS, SERVER_EFFECT, AnimationsDbusAnimationEffectSkeleton)

G_DECLARE_INTERFACE (AnimationsDbusServerEffectBridge, animations_dbus_server_effect_bridge, ANIMATIONS_DBUS, SERVER_EFFECT_BRIDGE, GObject)

G_DECLARE_INTERFACE (AnimationsDbusServerSurfaceAttachedEffect, animations_dbus_server_surface_attached_effect, ANIMATIONS_DBUS, SERVER_SURFACE_ATTACHED_EFFECT, GObject)

G_END_DECLS
