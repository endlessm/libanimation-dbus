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

#include <animations-dbus-server-effect-bridge-interface.h>
#include <animations-dbus-server-types.h>

G_BEGIN_DECLS

#define ANIMATIONS_DBUS_TYPE_SERVER_EFFECT_FACTORY animations_dbus_server_effect_factory_get_type ()

struct _AnimationsDbusServerEffectFactoryInterface
{
  GTypeInterface parent_iface;

  AnimationsDbusServerEffectBridge * (*create_effect) (AnimationsDbusServerEffectFactory  *self,
                                                       const char                         *effect,
                                                       GVariant                           *settings,
                                                       GError                            **error);
};

AnimationsDbusServerEffectBridge * animations_dbus_server_effect_factory_create_effect (AnimationsDbusServerEffectFactory  *self,
                                                                                        const char                         *effect,
                                                                                        GVariant                           *settings,
                                                                                        GError                            **error);

G_END_DECLS
