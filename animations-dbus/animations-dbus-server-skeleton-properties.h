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

G_BEGIN_DECLS

void animations_dbus_emit_properties_changed_for_skeleton_properties (GDBusInterfaceSkeleton *skeleton,
                                                                      const char * const     *properties);

gboolean
animations_dbus_set_property_from_variant (GObject     *object,
                                           const char  *name,
                                           GVariant    *variant,
                                           GError     **error);

gboolean
animations_dbus_validate_property_from_variant (GObject     *object,
                                                const char  *name,
                                                GVariant    *variant,
                                                GError     **error);

GVariant *
animations_dbus_serialize_properties_to_variant (GObject *object);

GVariant *
animations_dbus_serialize_pspecs_to_variant (GObject *object);

G_END_DECLS
