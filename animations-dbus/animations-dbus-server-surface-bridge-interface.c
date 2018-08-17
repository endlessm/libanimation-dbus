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

#include <glib.h>

#include "animations-dbus-server-surface-bridge-interface.h"

G_DEFINE_INTERFACE (AnimationsDbusServerSurfaceBridge,
                    animations_dbus_server_surface_bridge,
                    G_TYPE_OBJECT)

static void
animations_dbus_server_surface_bridge_default_init (AnimationsDbusServerSurfaceBridgeInterface *iface G_GNUC_UNUSED)
{
}

/**
 * animations_dbus_server_surface_bridge_attach_effect:
 * @self: An #AnimationsDbusServerSurfaceBridge
 * @event: The event name to attach to
 * @effect: The #AnimationsDbusServerEffect to be attached to this #AnimationsDbusServerSurfaceBridge
 * @error: A #GError
 *
 * Attach an @effect to @self, creating the resources necessary to implement the effect
 * on @self. The return value is an opaque interface representing an object
 * containing those resources. If the resources cannot be created, %NULL is returned.
 *
 * Returns: (transfer full): A #AnimationsDbusServerSurfaceAttachedEffect representing the
 *          resources to implement the effect on @self, or %NULL with @error set in case of
 *          an error.
 */
AnimationsDbusServerSurfaceAttachedEffect *
animations_dbus_server_surface_bridge_attach_effect (AnimationsDbusServerSurfaceBridge  *self,
                                                     const char                         *event,
                                                     AnimationsDbusServerEffect         *effect,
                                                     GError                            **error)
{
  g_return_val_if_fail (ANIMATIONS_DBUS_IS_SERVER_SURFACE_BRIDGE (self), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  AnimationsDbusServerSurfaceBridgeInterface *iface = ANIMATIONS_DBUS_SERVER_SURFACE_BRIDGE_GET_IFACE (self);

  g_return_val_if_fail (iface->attach_effect != NULL, NULL);
  return iface->attach_effect (self, event, effect, error);
}

void
animations_dbus_server_surface_bridge_detach_effect (AnimationsDbusServerSurfaceBridge         *self,
                                                     const char                                *event,
                                                     AnimationsDbusServerSurfaceAttachedEffect *effect)
{
  g_return_if_fail (ANIMATIONS_DBUS_IS_SERVER_SURFACE_BRIDGE (self));

  AnimationsDbusServerSurfaceBridgeInterface *iface = ANIMATIONS_DBUS_SERVER_SURFACE_BRIDGE_GET_IFACE (self);

  g_return_if_fail (iface->detach_effect != NULL);
  iface->detach_effect (self, event, effect);
}

const char *
animations_dbus_server_surface_bridge_get_title (AnimationsDbusServerSurfaceBridge *self)
{
  g_return_val_if_fail (ANIMATIONS_DBUS_IS_SERVER_SURFACE_BRIDGE (self), NULL);

  AnimationsDbusServerSurfaceBridgeInterface *iface = ANIMATIONS_DBUS_SERVER_SURFACE_BRIDGE_GET_IFACE (self);

  g_return_val_if_fail (iface->get_title != NULL, NULL);
  return iface->get_title (self);
}

GVariant *
animations_dbus_server_surface_bridge_get_geometry (AnimationsDbusServerSurfaceBridge *self)
{
  g_return_val_if_fail (ANIMATIONS_DBUS_IS_SERVER_SURFACE_BRIDGE (self), NULL);

  AnimationsDbusServerSurfaceBridgeInterface *iface = ANIMATIONS_DBUS_SERVER_SURFACE_BRIDGE_GET_IFACE (self);

  g_return_val_if_fail (iface->get_geometry != NULL, NULL);
  return iface->get_geometry (self);
}

GVariant *
animations_dbus_server_surface_bridge_get_available_effects (AnimationsDbusServerSurfaceBridge *self)
{
  g_return_val_if_fail (ANIMATIONS_DBUS_IS_SERVER_SURFACE_BRIDGE (self), NULL);

  AnimationsDbusServerSurfaceBridgeInterface *iface = ANIMATIONS_DBUS_SERVER_SURFACE_BRIDGE_GET_IFACE (self);

  g_return_val_if_fail (iface->get_available_effects != NULL, NULL);
  return iface->get_available_effects (self);
}
