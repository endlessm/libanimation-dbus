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

#define ANIMATIONS_DBUS_TYPE_CLIENT_SURFACE animations_dbus_client_surface_get_type ()
G_DECLARE_FINAL_TYPE (AnimationsDbusClientSurface, animations_dbus_client_surface, ANIMATIONS_DBUS, CLIENT_SURFACE, GObject)

gboolean animations_dbus_client_surface_attach_effect_finish (AnimationsDbusClientSurface  *client_surface ,
                                                              GAsyncResult                 *result,
                                                              GError                      **error);

void animations_dbus_client_surface_attach_effect_async (AnimationsDbusClientSurface *surface,
                                                         const char                  *event,
                                                         AnimationsDbusClientEffect  *effect,
                                                         GCancellable                *cancellable,
                                                         GAsyncReadyCallback          callback,
                                                         gpointer                     user_data);

gboolean
animations_dbus_client_surface_attach_effect (AnimationsDbusClientSurface  *surface,
                                              const char                   *event,
                                              AnimationsDbusClientEffect   *effect,
                                              GError                      **error);

gboolean
animations_dbus_client_surface_detach_effect_finish (AnimationsDbusClientSurface  *client_surface,
                                                     GAsyncResult                 *result,
                                                     GError                      **error);

void animations_dbus_client_surface_detach_effect_async (AnimationsDbusClientSurface *surface,
                                                         const char                  *event,
                                                         AnimationsDbusClientEffect  *effect,
                                                         GCancellable                *cancellable,
                                                         GAsyncReadyCallback          callback,
                                                         gpointer                     user_data);

gboolean
animations_dbus_client_surface_detach_effect (AnimationsDbusClientSurface  *surface,
                                              const char                   *event,
                                              AnimationsDbusClientEffect   *effect,
                                              GError                      **error);

GHashTable *
animations_dbus_client_surface_list_available_effects_finish (AnimationsDbusClientSurface  *surface,
                                                              GAsyncResult                 *result,
                                                              GError                      **error);

void animations_dbus_client_surface_list_available_effects_async (AnimationsDbusClientSurface *surface,
                                                                  GCancellable                *cancellable,
                                                                  GAsyncReadyCallback          callback,
                                                                  gpointer                     user_data);

GHashTable *
animations_dbus_client_surface_list_available_effects (AnimationsDbusClientSurface  *surface,
                                                       GError                      **error);

AnimationsDbusClientSurface * animations_dbus_client_surface_new_for_proxy (AnimationsDbusAnimatableSurface *proxy);

G_END_DECLS
