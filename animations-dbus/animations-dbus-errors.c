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

#include <gio/gio.h>

#include "animations-dbus-errors.h"

static const GDBusErrorEntry animations_dbus_error_entries[] =
{
  { ANIMATIONS_DBUS_ERROR_SERVER_SURFACE_NOT_FOUND, "com.endlessm.Libanimation.SurfaceNotFound" },
  { ANIMATIONS_DBUS_ERROR_NAME_ALREADY_REGISTERED, "com.endlessm.Libanimation.NameAlreadyRegistered" },
  { ANIMATIONS_DBUS_ERROR_CONNECTION_FAILED, "com.endlessm.Libanimation.ConnectionFailed" },
  { ANIMATIONS_DBUS_ERROR_NO_SUCH_ANIMATION, "com.endlessm.Libanimation.NoSuchAnimation" },
  { ANIMATIONS_DBUS_ERROR_INVALID_SETTING, "com.endlessm.Libanimation.InvalidSetting" },
  { ANIMATIONS_DBUS_ERROR_UNSUPPORTED_EVENT_FOR_ANIMATION_SURFACE,
    "com.endlessm.Libanimation.UnsupportedEventForAnimationSurface" },
  { ANIMATIONS_DBUS_ERROR_UNSUPPORTED_EVENT_FOR_ANIMATION_EFFECT,
    "com.endlessm.Libanimation.UnsupportedEventForAnimationEffect" },
  { ANIMATIONS_DBUS_ERROR_INTERNAL_ERROR, "com.endlessm.Libanimation.InternalError" }
};

GQuark
animations_dbus_error_quark (void)
{
  static volatile gsize quark_volatile = 0;
  g_dbus_error_register_error_domain ("animation-dbus-error-quark",
                                      &quark_volatile,
                                      animations_dbus_error_entries,
                                      G_N_ELEMENTS (animations_dbus_error_entries));

  return (GQuark) quark_volatile;
}
