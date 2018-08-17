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
#include <animations-dbus-server-effect-bridge-interface.h>
#include <animations-dbus-server-types.h>

G_BEGIN_DECLS

#define ANIMATIONS_DBUS_TYPE_SERVER_EFFECT animations_dbus_server_effect_get_type ()


gboolean animations_dbus_server_effect_export (AnimationsDbusServerEffect  *server_effect,
                                               const char                  *object_path,
                                               GError                     **error);

const char * animations_dbus_server_effect_get_title (AnimationsDbusServerEffect *server_effect);

void animations_dbus_server_effect_destroy (AnimationsDbusServerEffect *server_effect);

AnimationsDbusServerEffect * animations_dbus_server_effect_new (GDBusConnection                  *connection,
                                                                AnimationsDbusServerEffectBridge *bridge,
                                                                const char                       *title,
                                                                GVariant                         *settings);

G_END_DECLS
