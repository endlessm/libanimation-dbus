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

#include <glib.h>

G_BEGIN_DECLS

/**
 * AnimationsDbusError:
 * @ANIMATIONS_DBUS_ERROR_SERVER_SURFACE_NOT_FOUND: The referred to surface pointer
 *                                                  was not found
 * @ANIMATIONS_DBUS_ERROR_NAME_ALREADY_REGISTERED: Client name was already registered
 * @ANIMATIONS_DBUS_ERROR_CONNECTION_FAILED: Connection to the bus failed
 * @ANIMATIONS_DBUS_ERROR_NO_SUCH_ANIMATION: No such animation at object path
 * @ANIMATIONS_DBUS_ERROR_INVALID_SETTING: Invalid setting configuration
 * @ANIMATIONS_DBUS_ERROR_UNSUPPORTED_EVENT_FOR_ANIMATION_EFFECT: The animation effect
 *                                                                being attached does not
 *                                                                support this event.
 * @ANIMATIONS_DBUS_ERROR_UNSUPPORTED_EVENT_FOR_ANIMATION_SURFACE: The surface does not
 *                                                                 support this event.
 * @ANIMATIONS_DBUS_ERROR_INTERNAL_ERROR: Unrecoverable internal error
 *
 * Error enumeration for domain related errors.
 */
typedef enum {
  ANIMATIONS_DBUS_ERROR_SERVER_SURFACE_NOT_FOUND,
  ANIMATIONS_DBUS_ERROR_NAME_ALREADY_REGISTERED,
  ANIMATIONS_DBUS_ERROR_CONNECTION_FAILED,
  ANIMATIONS_DBUS_ERROR_NO_SUCH_ANIMATION,
  ANIMATIONS_DBUS_ERROR_INVALID_SETTING,
  ANIMATIONS_DBUS_ERROR_UNSUPPORTED_EVENT_FOR_ANIMATION_EFFECT,
  ANIMATIONS_DBUS_ERROR_UNSUPPORTED_EVENT_FOR_ANIMATION_SURFACE,
  ANIMATIONS_DBUS_ERROR_INTERNAL_ERROR
} AnimationsDbusError;

#define ANIMATIONS_DBUS_ERROR animations_dbus_error_quark ()
GQuark animations_dbus_error_quark (void);

G_END_DECLS
