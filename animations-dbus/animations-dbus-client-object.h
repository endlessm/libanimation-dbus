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

#include <animations-dbus-client-effect.h>
#include <animations-dbus-objects.h>

G_BEGIN_DECLS

#define ANIMATIONS_DBUS_TYPE_CLIENT animations_dbus_client_get_type ()
G_DECLARE_FINAL_TYPE (AnimationsDbusClient, animations_dbus_client, ANIMATIONS_DBUS, CLIENT, GObject)

AnimationsDbusClientEffect * animations_dbus_client_create_animation_effect_finish (AnimationsDbusClient  *client,
                                                                                    GAsyncResult          *result,
                                                                                    GError               **error);

void animations_dbus_client_create_animation_effect_async (AnimationsDbusClient *client,
                                                           const char           *title,
                                                           const char           *name,
                                                           GVariant             *settings,
                                                           GCancellable         *cancellable,
                                                           GAsyncReadyCallback   callback,
                                                           gpointer              user_data);


AnimationsDbusClientEffect * animations_dbus_client_create_animation_effect (AnimationsDbusClient  *client,
                                                                             const char            *title,
                                                                             const char            *name,
                                                                             GVariant              *settings,
                                                                             GError               **error);

GPtrArray * animations_dbus_client_list_surfaces_finish (AnimationsDbusClient  *client,
                                                         GAsyncResult          *result,
                                                         GError               **error);

void animations_dbus_client_list_surfaces_async (AnimationsDbusClient *client,
                                                 GCancellable         *cancellable,
                                                 GAsyncReadyCallback   callback,
                                                 gpointer              user_data);

GPtrArray * animations_dbus_client_list_surfaces (AnimationsDbusClient  *client,
                                                  GError               **error);

AnimationsDbusClient * animations_dbus_client_new_finish (GObject       *source,
                                                          GAsyncResult  *result,
                                                          GError       **error);

void animations_dbus_client_new_async (GCancellable        *cancellable,
                                       GAsyncReadyCallback  callback,
                                       gpointer             user_data);


void animations_dbus_client_new_with_connection_async (GDBusConnection     *connection,
                                                       GCancellable        *cancellable,
                                                       GAsyncReadyCallback  callback,
                                                       gpointer             user_data);

AnimationsDbusClient * animations_dbus_client_new (GError **error);

AnimationsDbusClient * animations_dbus_client_new_with_connection (GDBusConnection  *connection,
                                                                   GError          **error);

G_END_DECLS
