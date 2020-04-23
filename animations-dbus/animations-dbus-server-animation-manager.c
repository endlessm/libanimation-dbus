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
#include "animations-dbus-server-animation-manager.h"
#include "animations-dbus-server-effect.h"
#include "animations-dbus-server-object.h"
#include "animations-dbus-server-surface.h"

struct _AnimationsDbusServerAnimationManager
{
  AnimationsDbusAnimationManagerSkeleton parent_instance;
};

typedef struct _AnimationsDbusServerAnimationManagerPrivate
{
  GDBusConnection                *connection;
  AnimationsDbusServer           *server;
  AnimationsDbusServerEffectFactory *effect_factory;

  GHashTable *animation_effects;  /* (key-type: guint) (value-type: AnimationsDbusServerEffect) */
  guint       animation_effect_serial;
} AnimationsDbusServerAnimationManagerPrivate;

static void animations_dbus_animation_manager_interface_init (AnimationsDbusAnimationManagerIface *iface);

G_DEFINE_TYPE_WITH_CODE (AnimationsDbusServerAnimationManager,
                         animations_dbus_server_animation_manager,
                         ANIMATIONS_DBUS_TYPE_ANIMATION_MANAGER_SKELETON,
                         G_ADD_PRIVATE (AnimationsDbusServerAnimationManager)
                         G_IMPLEMENT_INTERFACE (ANIMATIONS_DBUS_TYPE_ANIMATION_MANAGER,
                                                animations_dbus_animation_manager_interface_init))

enum {
  PROP_0,
  PROP_CONNECTION,
  PROP_SERVER,
  PROP_EFFECT_FACTORY,
  NPROPS
};

static GParamSpec *animations_dbus_server_animation_manager_props[NPROPS];

/**
 * animations_dbus_server_animation_manager_lookup_effect_by_id:
 * @server_animation_manager: An #AnimationsDbusServerAnimationManager.
 * @effect_id: The ID of the effect to find.
 * @error: A #GError.
 *
 * Return an #AnimationsDbusServerEffect for the given ID on
 * this #AnimationsDbusServerAnimationManager.
 *
 * Returns: (transfer none): An #AnimationsDbusServerEffect or %NULL with
 *          @error set if one does not exist for that ID on this @animation_manager.
 */
AnimationsDbusServerEffect *
animations_dbus_server_animation_manager_lookup_effect_by_id (AnimationsDbusServerAnimationManager  *server_animation_manager,
                                                              unsigned int                           effect_id,
                                                              GError                               **error)
{
  AnimationsDbusServerAnimationManagerPrivate *priv =
    animations_dbus_server_animation_manager_get_instance_private (server_animation_manager);
  AnimationsDbusServerEffect *server_animation_effect = NULL;

  if (!g_hash_table_lookup_extended (priv->animation_effects,
                                     GUINT_TO_POINTER (effect_id),
                                     NULL,
                                     (gpointer *) &server_animation_effect))
    {
      g_set_error (error,
                   ANIMATIONS_DBUS_ERROR,
                   ANIMATIONS_DBUS_ERROR_NO_SUCH_ANIMATION,
                   "No such animation with id %u",
                   effect_id);
      return NULL;
    }

  return server_animation_effect;
}

gboolean
animations_dbus_server_animation_manager_export (AnimationsDbusServerAnimationManager  *server_animation_manager,
                                                 const char                            *object_path,
                                                 GError                               **error)
{
  AnimationsDbusServerAnimationManagerPrivate *priv = animations_dbus_server_animation_manager_get_instance_private (server_animation_manager);

  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (server_animation_manager),
                                           priv->connection,
                                           object_path,
                                           error);
}

void
animations_dbus_server_animation_manager_unexport (AnimationsDbusServerAnimationManager *server_animation_manager)
{
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (server_animation_manager));
}

/**
 * animations_dbus_server_animation_manager_create_effect:
 * @server_animation_manager: An #AnimationsDbusServerAnimationManager.
 * @title: The title of the effect.
 * @name: The name of the effect to be created.
 * @settings: A #GVariant of settings forming the initial settings payload.
 * @error: A #GError.
 *
 * Create a #AnimationsDbusServerEffect for use by the
 * server side. The effect will appear on the bus but
 * will be managed by the caller.
 *
 * Returns: (transfer full): A #AnimationsDbusServerAnimationManager
 * or %NULL with @error set in case of an error.
 */
AnimationsDbusServerEffect *
animations_dbus_server_animation_manager_create_effect (AnimationsDbusServerAnimationManager  *server_animation_manager,
                                                        const char                            *title,
                                                        const char                            *name,
                                                        GVariant                              *settings,
                                                        GError                               **error)
{
  AnimationsDbusServerAnimationManagerPrivate *priv = animations_dbus_server_animation_manager_get_instance_private (server_animation_manager);
  g_autoptr(GError) local_error = NULL;

  g_autoptr(AnimationsDbusServerEffectBridge) effect_bridge =
    animations_dbus_server_effect_factory_create_effect (priv->effect_factory,
                                                         name,
                                                         settings,
                                                         error);

  if (effect_bridge == NULL)
    return NULL;

  g_autoptr(AnimationsDbusServerEffect) animation_effect =
    animations_dbus_server_effect_new (priv->connection,
                                       effect_bridge,
                                       title,
                                       settings);
  const char *animation_manager_object_path =
    g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (server_animation_manager));
  unsigned int available_serial = priv->animation_effect_serial++;
  g_autofree char *animation_effect_object_path = g_strdup_printf ("%s/AnimationEffect/%u",
                                                                   animation_manager_object_path,
                                                                   available_serial);

  if (!animations_dbus_server_effect_export (animation_effect,
                                             animation_effect_object_path,
                                             &local_error))
    {
      g_autofree char *printed_variant = g_variant_print (settings, TRUE);

      g_set_error (error,
                   ANIMATIONS_DBUS_ERROR,
                   ANIMATIONS_DBUS_ERROR_INTERNAL_ERROR,
                   "Could not register AnimationEffect "
                   "for title '%s', name '%s', settings '%s': %s",
                   title,
                   name,
                   printed_variant,
                   local_error->message);
      return NULL;
    }

  /* We always insert the AnimationsDbusServerAnimationEffect here since
   * it should be visible and able to be looked up by clients on the bus. */
  g_hash_table_insert (priv->animation_effects,
                       GUINT_TO_POINTER (available_serial),
                       g_object_ref (animation_effect));
  return g_steal_pointer (&animation_effect);
}

typedef gpointer (*ElementMapFunc) (gpointer);

static GPtrArray *
pointer_array_map (GPtrArray      *src,
                   ElementMapFunc  mapper,
                   GDestroyNotify  dst_free_func)
{
  g_autoptr(GPtrArray) dst = g_ptr_array_new_full (src->len, dst_free_func);

  for (size_t i = 0 ; i < src->len; ++i)
    g_ptr_array_add (dst, mapper (g_ptr_array_index (src, i)));

  return g_steal_pointer (&dst);
}

static gboolean
animations_dbus_server_animation_manager_list_surfaces (AnimationsDbusAnimationManager *animation_manager,
                                                        GDBusMethodInvocation          *invocation)
{
  AnimationsDbusServerAnimationManager *server_animation_manager =
    ANIMATIONS_DBUS_SERVER_ANIMATION_MANAGER (animation_manager);
  AnimationsDbusServerAnimationManagerPrivate *priv = animations_dbus_server_animation_manager_get_instance_private (server_animation_manager);
  GPtrArray *server_surfaces =
    animations_dbus_server_list_surfaces (priv->server);
  g_autoptr(GPtrArray) server_surface_object_paths =
    pointer_array_map (server_surfaces,
                       (ElementMapFunc) g_dbus_interface_skeleton_get_object_path,
                       NULL);

  g_ptr_array_add (server_surface_object_paths, NULL);

  animations_dbus_animation_manager_complete_list_surfaces (animation_manager,
                                                            invocation,
                                                            (const char * const *) server_surface_object_paths->pdata);
  return TRUE;
}

static gboolean
animations_dbus_server_animation_manager_create_animation_effect (AnimationsDbusAnimationManager *animation_manager,
                                                                  GDBusMethodInvocation          *invocation,
                                                                  const char                     *title,
                                                                  const char                     *name,
                                                                  GVariant                       *settings)
{
  AnimationsDbusServerAnimationManager *server_animation_manager =
    ANIMATIONS_DBUS_SERVER_ANIMATION_MANAGER (animation_manager);
  g_autoptr(GError) local_error = NULL;
  g_autoptr(AnimationsDbusServerEffect) server_effect =
    animations_dbus_server_animation_manager_create_effect (server_animation_manager,
                                                            title,
                                                            name,
                                                            settings,
                                                            &local_error);

  if (server_effect == NULL)
    {
      g_dbus_method_invocation_return_gerror (invocation,
                                              g_steal_pointer (&local_error));
      return TRUE;
    }

  animations_dbus_animation_manager_complete_create_animation_effect (animation_manager,
                                                                      invocation,
                                                                      g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (server_effect)));
  return TRUE;
}

static void
animations_dbus_animation_manager_interface_init (AnimationsDbusAnimationManagerIface *iface)
{
  iface->handle_list_surfaces = animations_dbus_server_animation_manager_list_surfaces;
  iface->handle_create_animation_effect = animations_dbus_server_animation_manager_create_animation_effect;
}

static void
animations_dbus_server_animation_manager_set_property (GObject      *object,
                                                       guint         prop_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec)
{
  AnimationsDbusServerAnimationManager *server_animation_manager =
    ANIMATIONS_DBUS_SERVER_ANIMATION_MANAGER (object);
  AnimationsDbusServerAnimationManagerPrivate *priv = animations_dbus_server_animation_manager_get_instance_private (server_animation_manager);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_clear_object (&priv->connection);
      priv->connection = g_value_dup_object (value);
      break;
    case PROP_SERVER:
      /* Observing reference, AnimationsDbusServer is expected to last
       * longer than AnimationsDbusServerAnimationManager */
      priv->server = g_value_get_object (value);
      break;
    case PROP_EFFECT_FACTORY:
      priv->effect_factory = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
animations_dbus_server_animation_manager_get_property (GObject    *object,
                                                       guint       prop_id,
                                                       GValue     *value,
                                                       GParamSpec *pspec)
{
  AnimationsDbusServerAnimationManager *server_animation_manager =
    ANIMATIONS_DBUS_SERVER_ANIMATION_MANAGER (object);
  AnimationsDbusServerAnimationManagerPrivate *priv =
    animations_dbus_server_animation_manager_get_instance_private (server_animation_manager);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, priv->connection);
      break;
    case PROP_SERVER:
      g_value_set_object (value, priv->server);
      break;
    case PROP_EFFECT_FACTORY:
      g_value_set_object (value, priv->effect_factory);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
unref_hash_table_and_destroy_all_server_effect_values (GHashTable *animation_effects)
{
  GHashTableIter iter;
  gpointer value;

  /* Need to manually call animations_dbus_server_effect_destroy on each
   * member of the hash table, since it is possible that language bindings
   * could still have a reference on the effects. Calling
   * animations_dbus_server_effect_destroy will ensure that the "destroy"
   * signal gets emitted and the effect is detached from any surfaces. */
  g_hash_table_iter_init (&iter, animation_effects);

  while (g_hash_table_iter_next (&iter, NULL, &value))
    animations_dbus_server_effect_destroy (ANIMATIONS_DBUS_SERVER_EFFECT (value));

  g_hash_table_unref (animation_effects);
}

static void
animations_dbus_server_animation_manager_finalize (GObject *object)
{
  AnimationsDbusServerAnimationManager *server_animation_manager = ANIMATIONS_DBUS_SERVER_ANIMATION_MANAGER (object);
  AnimationsDbusServerAnimationManagerPrivate *priv = animations_dbus_server_animation_manager_get_instance_private (server_animation_manager);

  g_clear_pointer (&priv->animation_effects, unref_hash_table_and_destroy_all_server_effect_values);

  G_OBJECT_CLASS (animations_dbus_server_animation_manager_parent_class)->finalize (object);
}

static void
animations_dbus_server_animation_manager_init (AnimationsDbusServerAnimationManager *animation_manager)
{
  AnimationsDbusServerAnimationManagerPrivate *priv = animations_dbus_server_animation_manager_get_instance_private (animation_manager);

  priv->animation_effects = g_hash_table_new_full (g_direct_hash,
                                                   g_direct_equal,
                                                   NULL,
                                                   g_object_unref);
}

static void
animations_dbus_server_animation_manager_class_init (AnimationsDbusServerAnimationManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = animations_dbus_server_animation_manager_get_property;
  object_class->set_property = animations_dbus_server_animation_manager_set_property;
  object_class->finalize = animations_dbus_server_animation_manager_finalize;

  animations_dbus_server_animation_manager_props[PROP_CONNECTION] =
    g_param_spec_object ("connection",
                         "DBus Connection",
                         "The DBus Connection this ServerAnimationManager is exported on",
                         G_TYPE_DBUS_CONNECTION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  animations_dbus_server_animation_manager_props[PROP_SERVER] =
    g_param_spec_object ("server",
                         "Server",
                         "The Server this ServerAnimationManager is for",
                         ANIMATIONS_DBUS_TYPE_SERVER,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  animations_dbus_server_animation_manager_props[PROP_EFFECT_FACTORY] =
    g_param_spec_object ("effect-factory",
                         "Effect Factory",
                         "The AnimationsDbusServerEffectFactory that will create the effects",
                         ANIMATIONS_DBUS_TYPE_SERVER_EFFECT_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     NPROPS,
                                     animations_dbus_server_animation_manager_props);
}

AnimationsDbusServerAnimationManager *
animations_dbus_server_animation_manager_new (GDBusConnection                   *connection,
                                              AnimationsDbusServer              *server,
                                              AnimationsDbusServerEffectFactory *effect_factory)
{
  return g_object_new (ANIMATIONS_DBUS_TYPE_SERVER_ANIMATION_MANAGER,
                       "connection", connection,
                       "server", server,
                       "effect-factory", effect_factory,
                       NULL);
}
