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

#include "animations-dbus-server-surface-attached-effect-interface.h"

G_DEFINE_INTERFACE (AnimationsDbusServerSurfaceAttachedEffect,
                    animations_dbus_server_surface_attached_effect,
                    G_TYPE_OBJECT)

static void
animations_dbus_server_surface_attached_effect_default_init (AnimationsDbusServerSurfaceAttachedEffectInterface *iface G_GNUC_UNUSED)
{
}

