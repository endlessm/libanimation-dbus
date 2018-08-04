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

#include "animations-dbus-server-effect-bridge-interface.h"

G_DEFINE_INTERFACE (AnimationsDbusServerEffectBridge,
                    animations_dbus_server_effect_bridge,
                    G_TYPE_OBJECT)

static void
animations_dbus_server_effect_bridge_default_init (AnimationsDbusServerEffectBridgeInterface *iface G_GNUC_UNUSED)
{
}

const char *
animations_dbus_server_effect_bridge_get_name (AnimationsDbusServerEffectBridge *self)
{
  g_return_val_if_fail (ANIMATIONS_DBUS_IS_SERVER_EFFECT_BRIDGE (self), NULL);

  AnimationsDbusServerEffectBridgeInterface *iface = ANIMATIONS_DBUS_SERVER_EFFECT_BRIDGE_GET_IFACE (self);

  g_return_val_if_fail (iface->get_name != NULL, NULL);
  return iface->get_name (self);
}
