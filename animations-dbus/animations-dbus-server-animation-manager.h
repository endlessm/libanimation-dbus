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

#include <animations-dbus-server-effect.h>
#include <animations-dbus-server-effect-factory-interface.h>
#include <animations-dbus-server-types.h>

G_BEGIN_DECLS

#define ANIMATIONS_DBUS_TYPE_SERVER_ANIMATION_MANAGER animations_dbus_server_animation_manager_get_type ()

AnimationsDbusServerEffect * animations_dbus_server_animation_manager_lookup_effect_by_id (AnimationsDbusServerAnimationManager  *server_animation_manager,
                                                                                           unsigned int                           effect_id,
                                                                                           GError                               **error);


AnimationsDbusServerEffect * animations_dbus_server_animation_manager_create_effect (AnimationsDbusServerAnimationManager  *server_animation_manager,
                                                                                     const char                            *title,
                                                                                     const char                            *name,
                                                                                     GVariant                              *settings,
                                                                                     GError                               **error);

gboolean animations_dbus_server_animation_manager_export (AnimationsDbusServerAnimationManager  *server_animation_manager,
                                                          const char                            *object_path,
                                                          GError                               **error);
void animations_dbus_server_animation_manager_unexport (AnimationsDbusServerAnimationManager *server_animation_manager);

AnimationsDbusServerAnimationManager * animations_dbus_server_animation_manager_new (GDBusConnection                   *connection,
                                                                                     AnimationsDbusServer              *server,
                                                                                     AnimationsDbusServerEffectFactory *factory);

G_END_DECLS
