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

#define _ANIMATIONS_DBUS_INSIDE_ANIMATIONS_DBUS_SERVER_H

/* Needs to be included first to break include cycles */
#include <animations-dbus-server-types.h>

#include <animations-dbus-errors.h>
#include <animations-dbus-server-animation-manager.h>
#include <animations-dbus-server-effect.h>
#include <animations-dbus-server-effect-bridge-interface.h>
#include <animations-dbus-server-effect-factory-interface.h>
#include <animations-dbus-server-object.h>
#include <animations-dbus-server-surface.h>
#include <animations-dbus-server-surface-attached-effect-interface.h>
#include <animations-dbus-server-surface-bridge-interface.h>
#include <animations-dbus-objects.h>
#include <animations-dbus-version.h>

#undef _ANIMATIONS_DBUS_INSIDE_ANIMATIONS_DBUS_SERVER_H

G_END_DECLS
