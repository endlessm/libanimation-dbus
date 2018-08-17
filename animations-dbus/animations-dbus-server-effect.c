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
#include "animations-dbus-objects.h"
#include "animations-dbus-server-effect.h"
#include "animations-dbus-server-skeleton-properties.h"

struct _AnimationsDbusServerEffect
{
  AnimationsDbusAnimationEffectSkeleton parent_instance;
};

typedef struct _AnimationsDbusServerEffectPrivate
{
  GDBusConnection                  *connection;
  AnimationsDbusServerEffectBridge *effect_bridge;

  char                             *title;
  gboolean                          is_destroyed;
} AnimationsDbusServerEffectPrivate;

static void animations_dbus_animation_effect_interface_init (AnimationsDbusAnimationEffectIface *iface);

G_DEFINE_TYPE_WITH_CODE (AnimationsDbusServerEffect,
                         animations_dbus_server_effect,
                         ANIMATIONS_DBUS_TYPE_ANIMATION_EFFECT_SKELETON,
                         G_ADD_PRIVATE (AnimationsDbusServerEffect)
                         G_IMPLEMENT_INTERFACE (ANIMATIONS_DBUS_TYPE_ANIMATION_EFFECT,
                                                animations_dbus_animation_effect_interface_init))

enum {
  PROP_0,
  PROP_CONNECTION,
  PROP_BRIDGE,
  PROP_NAME,
  PROP_TITLE,
  PROP_SETTINGS,
  PROP_SCHEMA,
  NPROPS
};

#define N_OWN_PROPS PROP_TITLE

static GParamSpec *animations_dbus_server_effect_props[N_OWN_PROPS];

enum {
  SIGNAL_DESTROYED,
  NSIGNALS
};

static unsigned int animations_dbus_server_effect_signals[NSIGNALS];

gboolean
animations_dbus_server_effect_export (AnimationsDbusServerEffect  *server_effect,
                                      const char                  *object_path,
                                      GError                     **error)
{
  AnimationsDbusServerEffectPrivate *priv = animations_dbus_server_effect_get_instance_private (server_effect);

  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (server_effect),
                                           priv->connection,
                                           object_path,
                                           error);
}

const char *
animations_dbus_server_effect_get_title (AnimationsDbusServerEffect *server_effect)
{
  AnimationsDbusServerEffectPrivate *priv = animations_dbus_server_effect_get_instance_private (server_effect);

  return priv->title;
}

/**
 * animations_dbus_server_effect_destroy:
 * @server_effect: An #AnimationsDbusServerEffect
 *
 * Cause the effect to be marked as "destroyed", which unexports it from
 * the bus and emits the "destroy" signal, notifying interested listeners
 * that the effect should be considered inert. It is safe to call this
 * function multiple times, since the destroy signal emission and
 * unexport process will only happen once.
 */
void
animations_dbus_server_effect_destroy (AnimationsDbusServerEffect *server_effect)
{
  AnimationsDbusServerEffectPrivate *priv = animations_dbus_server_effect_get_instance_private (server_effect);

  if (!priv->is_destroyed)
    g_signal_emit (server_effect,
                   animations_dbus_server_effect_signals[SIGNAL_DESTROYED],
                   0);

  /* XXX: Not ideal to have a check like this, but since animations_dbus_server_effect_destroy
   *      can be called from animations_dbus_server_effect_dispose (where our reference count
   *      would be zero), we need to have this check to avoid unexporting during the
   *      dispose phase. The second check is to make sure we do not unexport twice,
   *      which would raise a warning. */
  if (G_IS_DBUS_INTERFACE_SKELETON (server_effect) &&
      g_dbus_interface_skeleton_has_connection (G_DBUS_INTERFACE_SKELETON (server_effect),
                                                priv->connection))
    {
      g_dbus_interface_skeleton_unexport_from_connection (G_DBUS_INTERFACE_SKELETON (server_effect),
                                                          priv->connection);
    }

  priv->is_destroyed = TRUE;
}

static gboolean
animations_dbus_server_effect_delete (AnimationsDbusAnimationEffect *animation_effect,
                                      GDBusMethodInvocation         *invocation)
{
  AnimationsDbusServerEffect *server_effect = ANIMATIONS_DBUS_SERVER_EFFECT (animation_effect);

  animations_dbus_server_effect_destroy (server_effect);
  animations_dbus_animation_effect_complete_delete (animation_effect, invocation);

  return TRUE;
}

static gboolean
animations_dbus_server_effect_change_setting (AnimationsDbusAnimationEffect *animation_effect,
                                              GDBusMethodInvocation         *invocation,
                                              const char                    *name,
                                              GVariant                      *value)
{
  AnimationsDbusServerEffect *server_effect = ANIMATIONS_DBUS_SERVER_EFFECT (animation_effect);
  AnimationsDbusServerEffectPrivate *priv = animations_dbus_server_effect_get_instance_private (server_effect);
  g_autoptr(GVariant) unboxed = g_variant_ref_sink (g_variant_get_variant (value));
  g_autoptr(GError) local_error = NULL;

  if (!animations_dbus_set_property_from_variant (G_OBJECT (priv->effect_bridge),
                                                  name,
                                                  unboxed,
                                                  &local_error))
    {
      g_dbus_method_invocation_return_gerror (invocation, g_steal_pointer (&local_error));
      return TRUE;
    }

  const char *props[] = { "settings", NULL };
  animations_dbus_emit_properties_changed_for_skeleton_properties (G_DBUS_INTERFACE_SKELETON (animation_effect),
                                                                   props);

  animations_dbus_animation_effect_complete_change_setting (animation_effect, invocation);
  return TRUE;
}

static void
animations_dbus_animation_effect_interface_init (AnimationsDbusAnimationEffectIface *effect)
{
  effect->handle_delete = animations_dbus_server_effect_delete;
  effect->handle_change_setting = animations_dbus_server_effect_change_setting;
}

static void
animations_dbus_server_effect_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  AnimationsDbusServerEffect *server_effect = ANIMATIONS_DBUS_SERVER_EFFECT (object);
  AnimationsDbusServerEffectPrivate *priv = animations_dbus_server_effect_get_instance_private (server_effect);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      priv->connection = g_value_dup_object (value);
      break;
    case PROP_BRIDGE:
      priv->effect_bridge = g_value_dup_object (value);
      break;
    case PROP_TITLE:
      priv->title = g_value_dup_string (value);
      break;
    case PROP_SETTINGS:
      {
        GVariant *settings = g_value_get_variant (value);
        GVariantIter iter;
        g_autoptr(GError) local_error = NULL;
        char *key;
        GVariant *value;

        g_variant_iter_init (&iter, settings);
        while (g_variant_iter_next (&iter, "{sv}", &key, &value))
          {
            g_autofree char *owned_key = g_steal_pointer (&key);
            g_autoptr(GVariant) owned_value = g_steal_pointer (&value);
            if (!animations_dbus_set_property_from_variant (G_OBJECT (priv->effect_bridge),
                                                            owned_key,
                                                            owned_value,
                                                            &local_error))
              {
                g_autofree char *printed_value = g_variant_print (owned_value, TRUE);
                g_warning ("Could not set property '%s' to value '%s' on animation '%s': %s",
                           owned_key,
                           printed_value,
                           animations_dbus_server_effect_bridge_get_name (priv->effect_bridge),
                           local_error->message);
                continue;
              }
          }
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
animations_dbus_server_effect_get_property (GObject    *object,
                                            guint       prop_id,
                                            GValue     *value,
                                            GParamSpec *pspec)
{
  AnimationsDbusServerEffect *server_effect = ANIMATIONS_DBUS_SERVER_EFFECT (object);
  AnimationsDbusServerEffectPrivate *priv = animations_dbus_server_effect_get_instance_private (server_effect);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, priv->connection);
      break;
    case PROP_BRIDGE:
      g_value_set_object (value, priv->effect_bridge);
      break;
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_NAME:
      g_value_set_string (value, animations_dbus_server_effect_bridge_get_name (priv->effect_bridge));
      break;
    case PROP_SETTINGS:
      g_value_take_variant (value,
                           animations_dbus_serialize_properties_to_variant (G_OBJECT (priv->effect_bridge)));
      break;
    case PROP_SCHEMA:
      g_value_take_variant (value,
                           animations_dbus_serialize_pspecs_to_variant (G_OBJECT (priv->effect_bridge)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
animations_dbus_server_effect_dispose (GObject *object)
{
  AnimationsDbusServerEffect *server_effect = ANIMATIONS_DBUS_SERVER_EFFECT (object);
  AnimationsDbusServerEffectPrivate *priv = animations_dbus_server_effect_get_instance_private (server_effect);

  /* Destroy the effect and emit the destroy signal now which will
   * cause the effect to be detached from any surfaces it is attached to. */
  animations_dbus_server_effect_destroy (server_effect);

  g_clear_object (&priv->effect_bridge);

  G_OBJECT_CLASS (animations_dbus_server_effect_parent_class)->dispose (object);
}

static void
animations_dbus_server_effect_finalize (GObject *object)
{
  AnimationsDbusServerEffect *server_effect = ANIMATIONS_DBUS_SERVER_EFFECT (object);
  AnimationsDbusServerEffectPrivate *priv = animations_dbus_server_effect_get_instance_private (server_effect);

  g_clear_pointer (&priv->title, g_free);

  G_OBJECT_CLASS (animations_dbus_server_effect_parent_class)->finalize (object);
}

static void
animations_dbus_server_effect_init (AnimationsDbusServerEffect *server_effect G_GNUC_UNUSED)
{
}

static void
animations_dbus_server_effect_class_init (AnimationsDbusServerEffectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = animations_dbus_server_effect_get_property;
  object_class->set_property = animations_dbus_server_effect_set_property;
  object_class->dispose = animations_dbus_server_effect_dispose;
  object_class->finalize = animations_dbus_server_effect_finalize;

  animations_dbus_server_effect_props[PROP_CONNECTION] =
    g_param_spec_object ("connection",
                         "A GDBusConnection",
                         "The GDBusConnection this effect is exported on",
                         G_TYPE_DBUS_CONNECTION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  animations_dbus_server_effect_props[PROP_BRIDGE] =
    g_param_spec_object ("bridge",
                         "An AnimationsDbusServerEffectBridge",
                         "The AnimationsDbusServerEffectBridge that defines the effect",
                         ANIMATIONS_DBUS_TYPE_SERVER_EFFECT_BRIDGE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  animations_dbus_server_effect_props[PROP_NAME] =
    g_param_spec_string ("name",
                         "Effect name",
                         "The name of this effect",
                         "",
                         G_PARAM_READABLE);

  g_object_class_install_properties (object_class,
                                     N_OWN_PROPS,
                                     animations_dbus_server_effect_props);

  g_object_class_override_property (object_class,
                                    PROP_TITLE,
                                    "title");
  g_object_class_override_property (object_class,
                                    PROP_SETTINGS,
                                    "settings");
  g_object_class_override_property (object_class,
                                    PROP_SCHEMA,
                                    "schema");

  animations_dbus_server_effect_signals[SIGNAL_DESTROYED] =
    g_signal_new ("destroyed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
}

AnimationsDbusServerEffect *
animations_dbus_server_effect_new (GDBusConnection                  *connection,
                                   AnimationsDbusServerEffectBridge *bridge,
                                   const char                       *title,
                                   GVariant                         *settings)
{
  return g_object_new (ANIMATIONS_DBUS_TYPE_SERVER_EFFECT,
                       "connection", connection,
                       "bridge", bridge,
                       "title", title,
                       "settings", settings,
                       NULL);
}
