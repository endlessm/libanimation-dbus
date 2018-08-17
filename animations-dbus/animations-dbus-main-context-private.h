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

/* Adapted from flatpak-utils-private.h */
typedef GMainContext GMainContextPopDefault;
static inline void
animations_dbus_main_context_pop_default_destroy (gpointer p)
{
  GMainContext *main_context = p;

  if (main_context)
    g_main_context_pop_thread_default (main_context);
}

static inline GMainContextPopDefault *
animations_dbus_main_context_new_default (void)
{
  GMainContext *main_context = g_main_context_new ();

  g_main_context_push_thread_default (main_context);
  return main_context;
}

void animations_dbus_got_async_func_result (GObject      *source,
                                            GAsyncResult *result,
                                            gpointer      user_data);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GMainContextPopDefault, animations_dbus_main_context_pop_default_destroy)
