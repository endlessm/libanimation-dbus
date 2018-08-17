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

#include "animations-dbus-client-effect.h"
#include "animations-dbus-client-surface.h"
#include "animations-dbus-errors.h"
#include "animations-dbus-main-context-private.h"
#include "animations-dbus-objects.h"

struct _AnimationsDbusClientSurface
{
  GObject parent_instance;
};

typedef struct _AnimationsDbusClientSurfacePrivate
{
  AnimationsDbusAnimatableSurface *proxy;
} AnimationsDbusClientSurfacePrivate;

G_DEFINE_TYPE_WITH_PRIVATE (AnimationsDbusClientSurface,
                            animations_dbus_client_surface,
                            G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_PROXY,
  PROP_TITLE,
  PROP_GEOMETRY,
  PROP_EFFECTS,
  NPROPS
};

static GParamSpec *animations_dbus_client_surface_properties[NPROPS];

/**
 * animations_dbus_client_surface_list_available_effects_finish:
 * @surface: An #AnimationsDbusClientSurface
 * @result: A #GAsyncResult
 * @error: A #GError
 *
 * Finish the call to animations_dbus_client_surface_list_available_effects and
 * return the available effects for each event.
 *
 * Returns: (transfer container) (element-type utf8 GStrv): A #GHashTable mapping
 *          event names to a list of possible animation names that can be used
 *          when attaching effects to that event.
 */
GHashTable *
animations_dbus_client_surface_list_available_effects_finish (AnimationsDbusClientSurface  *surface G_GNUC_UNUSED,
                                                              GAsyncResult                 *result,
                                                              GError                      **error)
{
  g_autoptr(GTask) task = G_TASK (result);
  return g_task_propagate_pointer (task, error);
}

static GHashTable *
available_effects_variant_to_hash_table (GVariant *available_effects_variant)
{
  g_autoptr(GHashTable) ht = g_hash_table_new_full (g_str_hash,
                                                    g_str_equal,
                                                    g_free,
                                                    (GDestroyNotify) g_strfreev);
  GVariantIter iter;
  char *key;
  GVariant *value;

  g_variant_iter_init (&iter, available_effects_variant);

  while (g_variant_iter_next (&iter, "{sv}", &key, &value))
    {
      g_autofree char *key_ref = g_steal_pointer (&key);
      g_autoptr(GVariant) value_ref = g_steal_pointer (&value);
      g_auto(GStrv) strv = (GStrv) g_variant_dup_strv (value_ref, NULL);

      g_hash_table_insert (ht,
                           (gpointer) g_steal_pointer (&key_ref),
                           (gpointer) g_steal_pointer (&strv));
    }

  return g_steal_pointer (&ht);
}

void
on_animations_dbus_client_surface_listed_available_effects (GObject      *source,
                                                            GAsyncResult *result,
                                                            gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  g_autoptr(GError) local_error = NULL;
  g_autoptr(GVariant) available_effects_variant = NULL;

  if (!animations_dbus_animatable_surface_call_list_effects_finish (ANIMATIONS_DBUS_ANIMATABLE_SURFACE (source),
                                                                    &available_effects_variant,
                                                                    result,
                                                                    &local_error))
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }


  g_task_return_pointer (task,
                         available_effects_variant_to_hash_table (available_effects_variant),
                         (GDestroyNotify) g_hash_table_unref);
}


void
animations_dbus_client_surface_list_available_effects_async (AnimationsDbusClientSurface *surface,
                                                             GCancellable                *cancellable,
                                                             GAsyncReadyCallback          callback,
                                                             gpointer                     user_data)
{
  AnimationsDbusClientSurfacePrivate *priv =
    animations_dbus_client_surface_get_instance_private (surface);
  GTask *task = g_task_new (surface, cancellable, callback, user_data);

  animations_dbus_animatable_surface_call_list_effects (ANIMATIONS_DBUS_ANIMATABLE_SURFACE (priv->proxy),
                                                        cancellable,
                                                        on_animations_dbus_client_surface_listed_available_effects,
                                                        task);
}

/**
 * animations_dbus_client_surface_list_available_effects:
 * @surface: An #AnimationsDbusClientSurface
 * @error: A #GError
 *
 * Synchronously get a listing of available effects for each event.
 *
 * Returns: (transfer container) (element-type utf8 GStrv): A #GHashTable mapping
 *          event names to a list of possible animation names that can be used
 *          when attaching effects to that event.
 */
GHashTable *
animations_dbus_client_surface_list_available_effects (AnimationsDbusClientSurface  *surface,
                                                       GError                      **error)
{
  g_autoptr(GMainContextPopDefault) context = animations_dbus_main_context_new_default ();
  g_autoptr(GAsyncResult) result = NULL;

  animations_dbus_client_surface_list_available_effects_async (surface,
                                                               NULL,
                                                               animations_dbus_got_async_func_result,
                                                               &result);

  while (result == NULL)
    g_main_context_iteration (context, TRUE);

  return animations_dbus_client_surface_list_available_effects_finish (surface,
                                                                       g_steal_pointer (&result),
                                                                       error);
}

gboolean
animations_dbus_client_surface_detach_effect_finish (AnimationsDbusClientSurface  *client_surface G_GNUC_UNUSED,
                                                     GAsyncResult                 *result,
                                                     GError                      **error)
{
  g_autoptr(GTask) task = G_TASK (result);
  return g_task_propagate_boolean (task, error);
}

static void
on_animations_dbus_client_surface_detached_effect (GObject      *source G_GNUC_UNUSED,
                                                   GAsyncResult *result,
                                                   gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  AnimationsDbusClientSurface *client_surface = ANIMATIONS_DBUS_CLIENT_SURFACE (g_task_get_task_data (task));
  AnimationsDbusClientSurfacePrivate *priv =
    animations_dbus_client_surface_get_instance_private (client_surface);
  g_autoptr(GError) local_error = NULL;

  if (!animations_dbus_animatable_surface_call_detach_animation_effect_finish (priv->proxy,
                                                                               result,
                                                                               &local_error))
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  g_task_return_boolean (task, TRUE);
}

void
animations_dbus_client_surface_detach_effect_async (AnimationsDbusClientSurface *surface,
                                                    const char                  *event,
                                                    AnimationsDbusClientEffect  *effect,
                                                    GCancellable                *cancellable,
                                                    GAsyncReadyCallback          callback,
                                                    gpointer                     user_data)
{
  AnimationsDbusClientSurfacePrivate *priv =
    animations_dbus_client_surface_get_instance_private (surface);
  GTask *task = g_task_new (surface, cancellable, callback, user_data);

  g_task_set_task_data (task, surface, NULL);

  animations_dbus_animatable_surface_call_detach_animation_effect (priv->proxy,
                                                                   event,
                                                                   animations_dbus_client_effect_get_object_path (effect),
                                                                   cancellable,
                                                                   on_animations_dbus_client_surface_detached_effect,
                                                                   task);
}

gboolean
animations_dbus_client_surface_detach_effect (AnimationsDbusClientSurface  *surface,
                                              const char                   *event,
                                              AnimationsDbusClientEffect   *effect,
                                              GError                      **error)
{
  g_autoptr(GMainContextPopDefault) context = animations_dbus_main_context_new_default ();
  g_autoptr(GAsyncResult) result = NULL;

  animations_dbus_client_surface_detach_effect_async (surface,
                                                      event,
                                                      effect,
                                                      NULL,
                                                      animations_dbus_got_async_func_result,
                                                      &result);

  while (result == NULL)
    g_main_context_iteration (context, TRUE);

  return animations_dbus_client_surface_detach_effect_finish (surface,
                                                              g_steal_pointer (&result),
                                                              error);
}

gboolean
animations_dbus_client_surface_attach_effect_finish (AnimationsDbusClientSurface  *client_surface G_GNUC_UNUSED,
                                                     GAsyncResult                 *result,
                                                     GError                      **error)
{
  g_autoptr(GTask) task = G_TASK (result);
  return g_task_propagate_boolean (task, error);
}

static void
on_animations_dbus_client_surface_attached_effect (GObject      *source G_GNUC_UNUSED,
                                                   GAsyncResult *result,
                                                   gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  AnimationsDbusClientSurface *client_surface = ANIMATIONS_DBUS_CLIENT_SURFACE (g_task_get_task_data (task));
  AnimationsDbusClientSurfacePrivate *priv =
    animations_dbus_client_surface_get_instance_private (client_surface);
  g_autoptr(GError) local_error = NULL;

  if (!animations_dbus_animatable_surface_call_attach_animation_effect_finish (priv->proxy,
                                                                               result,
                                                                               &local_error))
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  g_task_return_boolean (task, TRUE);
}

void
animations_dbus_client_surface_attach_effect_async (AnimationsDbusClientSurface *surface,
                                                    const char                  *event,
                                                    AnimationsDbusClientEffect  *effect,
                                                    GCancellable                *cancellable,
                                                    GAsyncReadyCallback          callback,
                                                    gpointer                     user_data)
{
  AnimationsDbusClientSurfacePrivate *priv =
    animations_dbus_client_surface_get_instance_private (surface);
  GTask *task = g_task_new (surface, cancellable, callback, user_data);

  g_task_set_task_data (task, surface, NULL);

  animations_dbus_animatable_surface_call_attach_animation_effect (priv->proxy,
                                                                   event,
                                                                   animations_dbus_client_effect_get_object_path (effect),
                                                                   cancellable,
                                                                   on_animations_dbus_client_surface_attached_effect,
                                                                   task);
}

gboolean
animations_dbus_client_surface_attach_effect (AnimationsDbusClientSurface  *surface,
                                              const char                   *event,
                                              AnimationsDbusClientEffect   *effect,
                                              GError                      **error)
{
  g_autoptr(GMainContextPopDefault) context = animations_dbus_main_context_new_default ();
  g_autoptr(GAsyncResult) result = NULL;

  animations_dbus_client_surface_attach_effect_async (surface,
                                                      event,
                                                      effect,
                                                      NULL,
                                                      animations_dbus_got_async_func_result,
                                                      &result);

  while (result == NULL)
    g_main_context_iteration (context, TRUE);

  return animations_dbus_client_surface_attach_effect_finish (surface,
                                                              g_steal_pointer (&result),
                                                              error);
}

static void
animations_dbus_client_surface_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  AnimationsDbusClientSurface *client_surface = ANIMATIONS_DBUS_CLIENT_SURFACE (object);
  AnimationsDbusClientSurfacePrivate *priv =
    animations_dbus_client_surface_get_instance_private (client_surface);

  switch (prop_id)
    {
    case PROP_PROXY:
      priv->proxy = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
animations_dbus_client_surface_get_property (GObject      *object,
                                             unsigned int  prop_id,
                                             GValue       *value,
                                             GParamSpec   *pspec)
{
  AnimationsDbusClientSurface *client_surface = ANIMATIONS_DBUS_CLIENT_SURFACE (object);
  AnimationsDbusClientSurfacePrivate *priv =
    animations_dbus_client_surface_get_instance_private (client_surface);

  switch (prop_id)
    {
    case PROP_PROXY:
      g_value_set_object (value, priv->proxy);
      break;
    case PROP_TITLE:
      g_value_set_string (value,
                          animations_dbus_animatable_surface_get_title (ANIMATIONS_DBUS_ANIMATABLE_SURFACE (priv->proxy)));
      break;
    case PROP_GEOMETRY:
      g_value_set_variant (value,
                           animations_dbus_animatable_surface_get_geometry (ANIMATIONS_DBUS_ANIMATABLE_SURFACE (priv->proxy)));
      break;
    case PROP_EFFECTS:
      g_value_set_variant (value,
                           animations_dbus_animatable_surface_get_effects (ANIMATIONS_DBUS_ANIMATABLE_SURFACE (priv->proxy)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
animations_dbus_client_surface_dispose (GObject *object)
{
  AnimationsDbusClientSurface *client = ANIMATIONS_DBUS_CLIENT_SURFACE (object);
  AnimationsDbusClientSurfacePrivate *priv = animations_dbus_client_surface_get_instance_private (client);

  g_clear_object (&priv->proxy);

  G_OBJECT_CLASS (animations_dbus_client_surface_parent_class)->dispose (object);
}

static void
animations_dbus_client_surface_init (AnimationsDbusClientSurface *client G_GNUC_UNUSED)
{
}

static void
animations_dbus_client_surface_class_init (AnimationsDbusClientSurfaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = animations_dbus_client_surface_set_property;
  object_class->get_property = animations_dbus_client_surface_get_property;
  object_class->dispose = animations_dbus_client_surface_dispose;

  animations_dbus_client_surface_properties[PROP_PROXY] =
    g_param_spec_object ("proxy",
                         "Proxy",
                         "The DBus proxy for this animatable surface",
                         ANIMATIONS_DBUS_TYPE_ANIMATABLE_SURFACE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  animations_dbus_client_surface_properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title for this surface",
                         "",
                         G_PARAM_READABLE);

  animations_dbus_client_surface_properties[PROP_GEOMETRY] =
    g_param_spec_variant ("geometry",
                          "Geometry",
                          "The geometry for this surface",
                          G_VARIANT_TYPE ("(iiii)"),
                          g_variant_new ("(iiii)", 0, 0, 1, 1),
                          G_PARAM_READABLE);

  animations_dbus_client_surface_properties[PROP_EFFECTS] =
    g_param_spec_variant ("effects",
                          "Effects",
                          "A dictionary of effects for each event on this surface",
                          G_VARIANT_TYPE ("a{sv}"),
                          g_variant_dict_end (g_variant_dict_new (NULL)),
                          G_PARAM_READABLE);

  g_object_class_install_properties (object_class,
                                     NPROPS,
                                     animations_dbus_client_surface_properties);
}

AnimationsDbusClientSurface *
animations_dbus_client_surface_new_for_proxy (AnimationsDbusAnimatableSurface *surface_proxy)
{
  return g_object_new (ANIMATIONS_DBUS_TYPE_CLIENT_SURFACE,
                       "proxy", surface_proxy,
                       NULL);
}
