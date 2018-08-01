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
#include "animations-dbus-errors.h"
#include "animations-dbus-main-context-private.h"
#include "animations-dbus-objects.h"

struct _AnimationsDbusClientEffect
{
  GObject parent_instance;
};

typedef struct _AnimationsDbusClientEffectPrivate
{
  AnimationsDbusAnimatableSurface *proxy;
} AnimationsDbusClientEffectPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (AnimationsDbusClientEffect,
                            animations_dbus_client_effect,
                            G_TYPE_OBJECT)


enum {
  PROP_0,
  PROP_PROXY,
  PROP_TITLE,
  PROP_SETTINGS,
  PROP_SCHEMA,
  NPROPS
};

static GParamSpec *animations_dbus_client_effect_properties[NPROPS];

const char *
animations_dbus_client_effect_get_object_path (AnimationsDbusClientEffect *client_effect)
{
  AnimationsDbusClientEffectPrivate *priv =
    animations_dbus_client_effect_get_instance_private (client_effect);

  return g_dbus_proxy_get_object_path (G_DBUS_PROXY (priv->proxy));
}

gboolean
animations_dbus_client_effect_change_setting_finish (AnimationsDbusClientEffect  *client_effect G_GNUC_UNUSED,
                                                     GAsyncResult                *result,
                                                     GError                     **error)
{
  g_autoptr(GTask) task = G_TASK (result);
  return g_task_propagate_boolean (task, error);
}

static void
on_animations_dbus_client_effect_changed_setting (GObject      *source G_GNUC_UNUSED,
                                                  GAsyncResult *result,
                                                  gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  AnimationsDbusClientEffect *client_effect =
    ANIMATIONS_DBUS_CLIENT_EFFECT (g_task_get_task_data (task));
  AnimationsDbusClientEffectPrivate *priv =
    animations_dbus_client_effect_get_instance_private (client_effect);
  g_autoptr(GError) local_error = NULL;

  if (!animations_dbus_animation_effect_call_change_setting_finish (ANIMATIONS_DBUS_ANIMATION_EFFECT (priv->proxy),
                                                                    result,
                                                                    &local_error))
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  g_task_return_boolean (task, TRUE);
}

void
animations_dbus_client_effect_change_setting_async (AnimationsDbusClientEffect *client_effect,
                                                    const char                 *name,
                                                    GVariant                   *value,
                                                    GCancellable               *cancellable,
                                                    GAsyncReadyCallback         callback,
                                                    gpointer                    user_data)
{
  AnimationsDbusClientEffectPrivate *priv =
    animations_dbus_client_effect_get_instance_private (client_effect);
  GTask *task = g_task_new (client_effect, cancellable, callback, user_data);

  g_task_set_task_data (task, client_effect, NULL);

  animations_dbus_animation_effect_call_change_setting (ANIMATIONS_DBUS_ANIMATION_EFFECT (priv->proxy),
                                                        name,
                                                        g_variant_new_variant (value),
                                                        cancellable,
                                                        on_animations_dbus_client_effect_changed_setting,
                                                        task);
}

gboolean
animations_dbus_client_effect_change_setting (AnimationsDbusClientEffect  *client_effect,
                                              const char                  *name,
                                              GVariant                    *value,
                                              GError                     **error)
{
  g_autoptr(GMainContextPopDefault) context = animations_dbus_main_context_new_default ();
  g_autoptr(GAsyncResult) result = NULL;

  animations_dbus_client_effect_change_setting_async (client_effect,
                                                      name,
                                                      value,
                                                      NULL,
                                                      animations_dbus_got_async_func_result,
                                                      &result);

  while (result == NULL)
    g_main_context_iteration (context, TRUE);

  return animations_dbus_client_effect_change_setting_finish (client_effect,
                                                              g_steal_pointer (&result),
                                                              error);
}

static void
animations_dbus_client_effect_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  AnimationsDbusClientEffect *client_effect = ANIMATIONS_DBUS_CLIENT_EFFECT (object);
  AnimationsDbusClientEffectPrivate *priv =
    animations_dbus_client_effect_get_instance_private (client_effect);

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
animations_dbus_client_effect_get_property (GObject      *object,
                                            unsigned int  prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec)
{
  AnimationsDbusClientEffect *client_effect = ANIMATIONS_DBUS_CLIENT_EFFECT (object);
  AnimationsDbusClientEffectPrivate *priv =
    animations_dbus_client_effect_get_instance_private (client_effect);

  switch (prop_id)
    {
    case PROP_PROXY:
      g_value_set_object (value, priv->proxy);
      break;
    case PROP_TITLE:
      g_value_set_string (value,
                          animations_dbus_animation_effect_get_title (ANIMATIONS_DBUS_ANIMATION_EFFECT (priv->proxy)));
      break;
    case PROP_SETTINGS:
      g_value_set_variant (value,
                           animations_dbus_animation_effect_get_settings (ANIMATIONS_DBUS_ANIMATION_EFFECT (priv->proxy)));
      break;
    case PROP_SCHEMA:
      g_value_set_variant (value,
                           animations_dbus_animation_effect_get_schema (ANIMATIONS_DBUS_ANIMATION_EFFECT (priv->proxy)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
animations_dbus_client_effect_dispose (GObject *object)
{
  AnimationsDbusClientEffect *client = ANIMATIONS_DBUS_CLIENT_EFFECT (object);
  AnimationsDbusClientEffectPrivate *priv = animations_dbus_client_effect_get_instance_private (client);

  g_clear_object (&priv->proxy);

  G_OBJECT_CLASS (animations_dbus_client_effect_parent_class)->dispose (object);
}

static void
animations_dbus_client_effect_init (AnimationsDbusClientEffect *client G_GNUC_UNUSED)
{
}

static GVariant *
empty_vardict (void)
{
  GVariantDict vardict;

  g_variant_dict_init (&vardict, NULL);
  return g_variant_dict_end (&vardict);
}

static void
animations_dbus_client_effect_class_init (AnimationsDbusClientEffectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = animations_dbus_client_effect_set_property;
  object_class->get_property = animations_dbus_client_effect_get_property;
  object_class->dispose = animations_dbus_client_effect_dispose;

  animations_dbus_client_effect_properties[PROP_PROXY] =
    g_param_spec_object ("proxy",
                         "Proxy",
                         "The DBus proxy for this animation effect",
                         ANIMATIONS_DBUS_TYPE_ANIMATION_EFFECT,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  animations_dbus_client_effect_properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The title for this effect",
                         "",
                         G_PARAM_READABLE);

  animations_dbus_client_effect_properties[PROP_SETTINGS] =
    g_param_spec_variant ("settings",
                          "Settings",
                          "The settings for this effect",
                          G_VARIANT_TYPE ("a{sv}"),
                          empty_vardict (),
                          G_PARAM_READABLE);

  animations_dbus_client_effect_properties[PROP_SCHEMA] =
    g_param_spec_variant ("schema",
                          "Schema",
                          "The schema for this effect's settings",
                          G_VARIANT_TYPE ("a{sv}"),
                          empty_vardict (),
                          G_PARAM_READABLE);

  g_object_class_install_properties (object_class,
                                     NPROPS,
                                     animations_dbus_client_effect_properties);
}

AnimationsDbusClientEffect *
animations_dbus_client_effect_new_for_proxy (AnimationsDbusAnimationEffect *effect_proxy)
{
  return g_object_new (ANIMATIONS_DBUS_TYPE_CLIENT_EFFECT,
                       "proxy", effect_proxy,
                       NULL);
}
