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
#include <animations-dbus-server-animation-manager.h>
#include <animations-dbus-server-effect.h>
#include <animations-dbus-server-effect-factory-interface.h>
#include <animations-dbus-server-surface-bridge-interface.h>
#include <animations-dbus-server-types.h>

G_BEGIN_DECLS

#define ANIMATIONS_DBUS_TYPE_SERVER animations_dbus_server_get_type ()

GPtrArray * animations_dbus_server_list_surfaces (AnimationsDbusServer *server);

AnimationsDbusServerEffect * animations_dbus_server_lookup_animation_effect_by_ids (AnimationsDbusServer  *server,
                                                                                    unsigned int           animation_manager_id,
                                                                                    unsigned int           animation_effect_id,
                                                                                    GError               **error);

AnimationsDbusServerSurface * animations_dbus_server_register_surface (AnimationsDbusServer               *server,
                                                                       AnimationsDbusServerSurfaceBridge  *bridge,
                                                                       GError                            **error);

gboolean animations_dbus_server_unregister_surface (AnimationsDbusServer         *server,
                                                    AnimationsDbusServerSurface  *server_surface,
                                                    GError                      **error);

AnimationsDbusServerAnimationManager * animations_dbus_server_create_animation_manager (AnimationsDbusServer  *server,
                                                                                        GError               **error);

AnimationsDbusServer * animations_dbus_server_new_finish (GObject       *source,
                                                          GAsyncResult  *result,
                                                          GError       **error);

void animations_dbus_server_new_async (AnimationsDbusServerEffectFactory *factory,
                                       GCancellable                      *cancellable,
                                       GAsyncReadyCallback                callback,
                                       gpointer                           user_data);

void animations_dbus_server_new_with_connection_async (AnimationsDbusServerEffectFactory *factory,
                                                       GDBusConnection                   *connection,
                                                       GCancellable                      *cancellable,
                                                       GAsyncReadyCallback                callback,
                                                       gpointer                           user_data);

G_END_DECLS
