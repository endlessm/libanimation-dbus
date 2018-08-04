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
#include "animations-dbus-server-effect-factory-interface.h"

G_DEFINE_INTERFACE (AnimationsDbusServerEffectFactory,
                    animations_dbus_server_effect_factory,
                    G_TYPE_OBJECT)

static void
animations_dbus_server_effect_factory_default_init (AnimationsDbusServerEffectFactoryInterface *iface G_GNUC_UNUSED)
{
}

/**
 * animations_dbus_server_effect_factory_create_effect:
 * @self: An #AnimationsDbusServerEffectFactory.
 * @effect: The effect name of the effect to create.
 * @settings: The initial settings for this effect.
 * @error: A #GError.
 *
 * Create the effect given by @effect with initial settings
 * given by @settings. If @effect does not refer to a known
 * effect, an error should be thrown. If @settings is not
 * a valid settings variant for the effect, an error should be
 * thrown.
 *
 * Returns: (transfer full): An #AnimationsDbusServerEffectBridge
 *          representing the implementation for the effect or
 *          %NULL with @error set in the case of an error.
 */
AnimationsDbusServerEffectBridge *
animations_dbus_server_effect_factory_create_effect (AnimationsDbusServerEffectFactory  *self,
                                                     const char                         *effect,
                                                     GVariant                           *settings,
                                                     GError                            **error)
{
  g_return_val_if_fail (ANIMATIONS_DBUS_IS_SERVER_EFFECT_FACTORY (self), NULL);

  AnimationsDbusServerEffectFactoryInterface *iface = ANIMATIONS_DBUS_SERVER_EFFECT_FACTORY_GET_IFACE (self);

  g_return_val_if_fail (iface->create_effect != NULL, NULL);
  return iface->create_effect (self, effect, settings, error);
}
