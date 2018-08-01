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
#include <glib-object.h>
#include <glib.h>

#include <animations-dbus-objects.h>

G_BEGIN_DECLS

#define ANIMATIONS_DBUS_TYPE_CLIENT_EFFECT animations_dbus_client_effect_get_type ()
G_DECLARE_FINAL_TYPE (AnimationsDbusClientEffect, animations_dbus_client_effect, ANIMATIONS_DBUS, CLIENT_EFFECT, GObject)

const char * animations_dbus_client_effect_get_object_path (AnimationsDbusClientEffect *client_effect);

gboolean animations_dbus_client_effect_change_setting_finish (AnimationsDbusClientEffect  *client_effect,
                                                              GAsyncResult                *result,
                                                              GError                     **error);

void animations_dbus_client_effect_change_setting_async (AnimationsDbusClientEffect *client_effect,
                                                         const char                 *name,
                                                         GVariant                   *value,
                                                         GCancellable               *cancellable,
                                                         GAsyncReadyCallback         callback,
                                                         gpointer                    user_data);

gboolean animations_dbus_client_effect_change_setting (AnimationsDbusClientEffect  *client_effect,
                                                       const char                  *name,
                                                       GVariant                    *value,
                                                       GError                     **error);

AnimationsDbusClientEffect * animations_dbus_client_effect_new_for_proxy (AnimationsDbusAnimationEffect *proxy);

G_END_DECLS
