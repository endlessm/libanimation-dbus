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
#include "animations-dbus-server-object.h"
#include "animations-dbus-server-skeleton-properties.h"
#include "animations-dbus-server-surface.h"
#include "animations-dbus-server-surface-attached-effect-interface.h"
#include "animations-dbus-server-surface-bridge-interface.h"

struct _AnimationsDbusServerSurface
{
  AnimationsDbusAnimatableSurfaceSkeleton parent_instance;
};

typedef struct _AnimationsDbusServerSurfacePrivate
{
  GDBusConnection                   *connection;
  AnimationsDbusServer              *server;
  AnimationsDbusServerSurfaceBridge *bridge;

  GHashTable *attached_effects_for_events;  /* (key-type: utf8) (value-type: GQueue) */
} AnimationsDbusServerSurfacePrivate;

static void animations_dbus_animatable_surface_interface_init (AnimationsDbusAnimatableSurfaceIface *iface);

G_DEFINE_TYPE_WITH_CODE (AnimationsDbusServerSurface,
                         animations_dbus_server_surface,
                         ANIMATIONS_DBUS_TYPE_ANIMATABLE_SURFACE_SKELETON,
                         G_ADD_PRIVATE (AnimationsDbusServerSurface)
                         G_IMPLEMENT_INTERFACE (ANIMATIONS_DBUS_TYPE_ANIMATABLE_SURFACE,
                                                animations_dbus_animatable_surface_interface_init))

enum {
  PROP_0,
  PROP_CONNECTION,
  PROP_SERVER,
  PROP_BRIDGE,
  PROP_TITLE,
  PROP_GEOMETRY,
  PROP_EFFECTS
};

#define N_OWN_PROPS PROP_TITLE

static GParamSpec *animations_dbus_server_surface_props[N_OWN_PROPS];

typedef struct {
  AnimationsDbusServerEffect                *server_effect;
  AnimationsDbusServerSurfaceAttachedEffect *attached_effect;
} AttachedEffectInfo;

AttachedEffectInfo *
attached_effect_info_new (AnimationsDbusServerEffect                *server_effect,
                          AnimationsDbusServerSurfaceAttachedEffect *attached_effect)
{
  AttachedEffectInfo *info = g_new0 (AttachedEffectInfo, 1);

  info->server_effect = g_object_ref (server_effect);
  info->attached_effect = g_object_ref (attached_effect);

  return info;
}

void
attached_effect_info_free (AttachedEffectInfo *info)
{
  g_clear_object (&info->server_effect);
  g_clear_object (&info->attached_effect);

  g_free (info);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (AttachedEffectInfo, attached_effect_info_free)

static void
animations_dbus_server_surface_detach_animation_effect_from_all_events (AnimationsDbusServerSurface *server_surface,
                                                                        AnimationsDbusServerEffect  *server_animation_effect)
{
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);
  gpointer key, value;
  GHashTableIter iter;

  g_hash_table_iter_init (&iter, priv->attached_effects_for_events);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const char *event = key;
      GQueue *effects = value;
      GList *link = g_queue_peek_head_link (effects);

      for (; link != NULL; link = link->next)
        {
          AttachedEffectInfo *info = link->data;

          if (info->server_effect == server_animation_effect)
            {
              animations_dbus_server_surface_bridge_detach_effect (priv->bridge,
                                                                   event,
                                                                   info->attached_effect);

              /* Notify listeners that we've dettached the effect from this
               * event and that the effects property has changed now. */
              const char *props[] = { "effects", NULL };
              animations_dbus_emit_properties_changed_for_skeleton_properties (G_DBUS_INTERFACE_SKELETON (server_surface),
                                                                               props);

              g_queue_delete_link (effects, link);
              attached_effect_info_free (info);
              break;
            }
        }
    }
}

static void
on_server_animation_effect_destroyed (AnimationsDbusServerEffect *server_animation_effect,
                                      gpointer                    user_data)
{
  AnimationsDbusServerSurface *server_surface = user_data;

  animations_dbus_server_surface_detach_animation_effect_from_all_events (server_surface,
                                                                          server_animation_effect);
}

typedef void (*QueuePushFunc) (GQueue *, gpointer);

static gboolean
animations_dbus_server_surface_attach_effect_with_queue_func (AnimationsDbusServerSurface  *server_surface,
                                                              const char                   *event,
                                                              AnimationsDbusServerEffect   *server_animation_effect,
                                                              QueuePushFunc                 push_func,
                                                              GError                      **error)
{
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);

  g_assert (push_func != NULL);

  /* Look for the event in the queue first to ensure that we don't
   * try and attach the same effect twice. O(N) for the number of
   * attached effects but N should be quite small. */
  GQueue *attached_effects_for_event = g_hash_table_lookup (priv->attached_effects_for_events, event);

  if (attached_effects_for_event == NULL)
    {
      attached_effects_for_event = g_queue_new ();

      g_hash_table_insert (priv->attached_effects_for_events,
                           g_strdup (event),
                           attached_effects_for_event);
    }

  for (GList *link = g_queue_peek_head_link (attached_effects_for_event);
       link != NULL;
       link = link->next)
    {
      AttachedEffectInfo *info = link->data;

      if (info->server_effect == server_animation_effect)
        return TRUE;
    }

  /* Now create an AttachedEffect struct, which represents our attempt
   * to attach this effect to the ServerSurfaceBridge. If the effect
   * cannot be attached to the SurfaceSurfaceBridge, this function
   * returns %NULL with @error set and we return accordingly. */
  g_autoptr(AnimationsDbusServerSurfaceAttachedEffect) attached_effect =
    animations_dbus_server_surface_bridge_attach_effect (priv->bridge,
                                                         event,
                                                         server_animation_effect,
                                                         error);

  if (attached_effect == NULL)
    return FALSE;

  push_func (attached_effects_for_event,
             attached_effect_info_new (server_animation_effect,
                                       attached_effect));

  /* Notify listeners that we've attached the effect to this
   * event and that the effects property has changed now. */
  const char *props[] = { "effects", NULL };
  animations_dbus_emit_properties_changed_for_skeleton_properties (G_DBUS_INTERFACE_SKELETON (server_surface),
                                                                   props);

  /* Watch for the effect to be destroyed. When it is deleted
   * we'll need to detach it from the surface too */
  g_signal_connect_object (server_animation_effect,
                           "destroyed",
                           G_CALLBACK (on_server_animation_effect_destroyed),
                           server_surface,
                           G_CONNECT_AFTER);

  return TRUE;
}

gboolean
animations_dbus_server_surface_export (AnimationsDbusServerSurface  *server_surface,
                                       const char                   *object_path,
                                       GError                      **error)
{
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);

  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (server_surface),
                                           priv->connection,
                                           object_path,
                                           error);
}

void
animations_dbus_server_surface_unexport (AnimationsDbusServerSurface *server_surface)
{
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);
  g_dbus_interface_skeleton_unexport_from_connection (G_DBUS_INTERFACE_SKELETON (server_surface),
                                                      priv->connection);
}

gboolean
animations_dbus_server_surface_attach_animation_effect_with_server_priority (AnimationsDbusServerSurface  *server_surface,
                                                                             const char                   *event,
                                                                             AnimationsDbusServerEffect   *server_animation_effect,
                                                                             GError                      **error)
{
  /* Other effects take priority over this one, since server effects
   * can be overridden by the user. */
  return animations_dbus_server_surface_attach_effect_with_queue_func (server_surface,
                                                                       event,
                                                                       server_animation_effect,
                                                                       g_queue_push_tail,
                                                                       error);
}

/**
 * animations_dbus_server_surface_highest_priority_attached_effect_for_event:
 * @server_surface: The #AnimationsDbusServerSurface with the attached effects.
 * @event: The event to get the highest priority effect on.
 *
 * Check the attached effects for @event on @surface and get the highest priority
 * effect, returning its #AnimationsDbusServerSurfaceAttachedEffect (private data
 * for the effect).
 *
 * Returns: (transfer none): The #AnimationsDbusServerSurfaceAttachedEffect for the highest
 *          priority effect on @surface for @event, or %NULL if no events are attached to
 *          @effect.
 */
AnimationsDbusServerSurfaceAttachedEffect *
animations_dbus_server_surface_highest_priority_attached_effect_for_event (AnimationsDbusServerSurface *server_surface,
                                                                           const char                  *event)
{
  AnimationsDbusServerSurfacePrivate *priv =
    animations_dbus_server_surface_get_instance_private (server_surface);
  GQueue *attached_effects_for_event = g_hash_table_lookup (priv->attached_effects_for_events, event);

  if (attached_effects_for_event)
    {
      AttachedEffectInfo *info = g_queue_peek_head (attached_effects_for_event);

      if (info == NULL)
        return NULL;

      return info->attached_effect;
    }

  return NULL;
}

void
animations_dbus_server_surface_emit_geometry_changed (AnimationsDbusServerSurface *server_surface)
{
  const char *props[] = { "geometry", NULL };

  animations_dbus_emit_properties_changed_for_skeleton_properties (G_DBUS_INTERFACE_SKELETON (server_surface),
                                                                   props);
}

void
animations_dbus_server_surface_emit_title_changed (AnimationsDbusServerSurface *server_surface)
{
  const char *props[] = { "title", NULL };

  animations_dbus_emit_properties_changed_for_skeleton_properties (G_DBUS_INTERFACE_SKELETON (server_surface),
                                                                   props);
}

/* /com/endlessm/Libanimation/AnimationManager/N/AnimationEffect/M */
#define EFFECT_PATH_EXPECTED_COMPONENTS 8
#define EFFECT_PATH_ANIMATION_MANAGER_INDEX 5
#define EFFECT_PATH_ANIMATION_EFFECT_INDEX 7

static gboolean
parse_effect_path (const char    *effect_path,
                   unsigned int  *out_animation_manager_id,
                   unsigned int  *out_animation_effect_id,
                   GError       **error)
{
  g_return_val_if_fail (out_animation_manager_id != NULL, FALSE);
  g_return_val_if_fail (out_animation_effect_id != NULL, FALSE);

  g_auto(GStrv) effect_path_components = g_strsplit (effect_path, "/", 0);
  unsigned int path_length = g_strv_length (effect_path_components);
  guint64 animation_manager_id64 = 0;
  guint64 animation_effect_id64 = 0;
  g_autoptr(GError) local_error = NULL;

  if (path_length != EFFECT_PATH_EXPECTED_COMPONENTS)
    {
      g_set_error (error,
                   ANIMATIONS_DBUS_ERROR,
                   ANIMATIONS_DBUS_ERROR_NO_SUCH_ANIMATION,
                   "Expected animation path '%s' to have %u components, but "
                   "had %u components",
                   effect_path,
                   EFFECT_PATH_EXPECTED_COMPONENTS,
                   path_length);
      return FALSE;
    }

  if (!g_ascii_string_to_unsigned (effect_path_components[EFFECT_PATH_ANIMATION_MANAGER_INDEX],
                                   10,
                                   0,
                                   G_MAXUINT,
                                   &animation_manager_id64,
                                   &local_error))
    {
      g_set_error (error,
                   ANIMATIONS_DBUS_ERROR,
                   ANIMATIONS_DBUS_ERROR_NO_SUCH_ANIMATION,
                   "Expected animation path component '%s' to be an "
                   "unsigned integer: %s",
                   effect_path_components[EFFECT_PATH_ANIMATION_MANAGER_INDEX],
                   local_error->message);
      return FALSE;
    }

  if (!g_ascii_string_to_unsigned (effect_path_components[EFFECT_PATH_ANIMATION_EFFECT_INDEX],
                                   10,
                                   0,
                                   G_MAXUINT,
                                   &animation_effect_id64,
                                   &local_error))
    {
      g_set_error (error,
                   ANIMATIONS_DBUS_ERROR,
                   ANIMATIONS_DBUS_ERROR_NO_SUCH_ANIMATION,
                   "Expected animation path component '%s' to be an "
                   "unsigned integer: %s",
                   effect_path_components[EFFECT_PATH_ANIMATION_EFFECT_INDEX],
                   local_error->message);
      return FALSE;
    }

  *out_animation_manager_id = (unsigned int) animation_manager_id64;
  *out_animation_effect_id = (unsigned int) animation_effect_id64;

  return TRUE;
}

static gboolean
animations_dbus_server_surface_attach_animation_effect (AnimationsDbusAnimatableSurface *animatable_surface,
                                                        GDBusMethodInvocation           *invocation,
                                                        const char                      *event,
                                                        const char                      *effect_path)
{
  AnimationsDbusServerSurface *server_surface = ANIMATIONS_DBUS_SERVER_SURFACE (animatable_surface);
  AnimationsDbusServerSurfacePrivate *priv =
    animations_dbus_server_surface_get_instance_private (server_surface);
  unsigned int animation_manager_id = 0;
  unsigned int animation_effect_id = 0;
  g_autoptr(GError) local_error = NULL;

  /* Validate that the passed in effect_path is a valid object path */
  if (!parse_effect_path (effect_path,
                          &animation_manager_id,
                          &animation_effect_id,
                          &local_error))
    {
      g_dbus_method_invocation_return_gerror (invocation,
                                              g_steal_pointer (&local_error));
      return TRUE;
    }

  AnimationsDbusServerEffect *server_animation_effect =
    animations_dbus_server_lookup_animation_effect_by_ids (priv->server,
                                                           animation_manager_id,
                                                           animation_effect_id,
                                                           &local_error);

  /* Insert the animation effect into the table */
  if (server_animation_effect == NULL)
    {
      g_dbus_method_invocation_return_gerror (invocation,
                                              g_steal_pointer (&local_error));
      return TRUE;
    }

  /* Newly attached effects take priority over old ones */
  if (!animations_dbus_server_surface_attach_effect_with_queue_func (server_surface,
                                                                     event,
                                                                     server_animation_effect,
                                                                     g_queue_push_head,
                                                                     &local_error))
    {
      g_dbus_method_invocation_return_gerror (invocation,
                                              g_steal_pointer (&local_error));
      return TRUE;
    }

  animations_dbus_animatable_surface_complete_attach_animation_effect (animatable_surface,
                                                                       invocation);
  return TRUE;
}

static gboolean
animations_dbus_server_surface_detach_animation_effect (AnimationsDbusAnimatableSurface *animatable_surface,
                                                        GDBusMethodInvocation           *invocation,
                                                        const char                      *event,
                                                        const char                      *effect_path)
{
  AnimationsDbusServerSurface *server_surface = ANIMATIONS_DBUS_SERVER_SURFACE (animatable_surface);
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);
  unsigned int animation_manager_id = 0;
  unsigned int animation_effect_id = 0;
  GQueue *attached_effects_for_events = NULL;
  g_autoptr(GError) local_error = NULL;

  /* Validate that the passed in effect_path is a valid object path */
  if (!parse_effect_path (effect_path,
                          &animation_manager_id,
                          &animation_effect_id,
                          &local_error))
    {
      g_dbus_method_invocation_return_gerror (invocation,
                                              g_steal_pointer (&local_error));
      return TRUE;
    }

  AnimationsDbusServerEffect *server_animation_effect =
    animations_dbus_server_lookup_animation_effect_by_ids (priv->server,
                                                           animation_manager_id,
                                                           animation_effect_id,
                                                           &local_error);

  /* Remove the animation effect from the table */
  if (server_animation_effect == NULL)
    {
      g_dbus_method_invocation_return_gerror (invocation,
                                              g_steal_pointer (&local_error));
      return TRUE;
    }

  attached_effects_for_events = g_hash_table_lookup (priv->attached_effects_for_events, event);

  /* Not attached, do nothing */
  if (attached_effects_for_events == NULL)
    {
      animations_dbus_animatable_surface_complete_detach_animation_effect (animatable_surface,
                                                                           invocation);
      return TRUE;
    }

  /* Search for the effect in the attached effects and remove it,
   * also notifying the bridge that the effect is to be detached. */
  for (GList *link = g_queue_peek_head_link (attached_effects_for_events);
       link != NULL;
       link = link->next)
    {
      AttachedEffectInfo *info = link->data;

      if (info->server_effect == server_animation_effect)
        {
          animations_dbus_server_surface_bridge_detach_effect (priv->bridge,
                                                               event,
                                                               info->attached_effect);

          /* Notify listeners that we've dettached the effect from this
           * event and that the effects property has changed now. */
          const char *props[] = { "effects", NULL };
          animations_dbus_emit_properties_changed_for_skeleton_properties (G_DBUS_INTERFACE_SKELETON (server_surface),
                                                                           props);

          g_queue_delete_link (attached_effects_for_events, link);
          attached_effect_info_free (info);
          break;
        }
    }

  animations_dbus_animatable_surface_complete_detach_animation_effect (animatable_surface,
                                                                       invocation);
  return TRUE;
}

static gboolean
animations_dbus_server_surface_list_animation_effects (AnimationsDbusAnimatableSurface *animatable_surface,
                                                       GDBusMethodInvocation           *invocation)
{
  AnimationsDbusServerSurface *server_surface = ANIMATIONS_DBUS_SERVER_SURFACE (animatable_surface);
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);
  g_autoptr(GVariant) available_effects =
    g_variant_ref_sink (animations_dbus_server_surface_bridge_get_available_effects (priv->bridge));

  animations_dbus_animatable_surface_complete_list_effects (animatable_surface,
                                                            invocation,
                                                            available_effects);
  return TRUE;
}

static void
animations_dbus_animatable_surface_interface_init (AnimationsDbusAnimatableSurfaceIface *iface)
{
  iface->handle_attach_animation_effect = animations_dbus_server_surface_attach_animation_effect;
  iface->handle_detach_animation_effect = animations_dbus_server_surface_detach_animation_effect;
  iface->handle_list_effects = animations_dbus_server_surface_list_animation_effects;
}

static void
animations_dbus_server_surface_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  AnimationsDbusServerSurface *server_surface = ANIMATIONS_DBUS_SERVER_SURFACE (object);
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      priv->connection = g_value_dup_object (value);
      break;
    case PROP_SERVER:
      /* Observing reference, AnimationsDbusServer is expected to last
       * longer than AnimationsDbusServerSurface */
      priv->server = g_value_get_object (value);
      break;
    case PROP_BRIDGE:
      priv->bridge = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static GVariant *
serialize_attached_effects_to_variant (GHashTable *effects_for_events)
{
  g_auto(GVariantDict) vardict;
  gpointer key, value;
  GHashTableIter iter;

  g_variant_dict_init (&vardict, NULL);

  g_hash_table_iter_init (&iter, effects_for_events);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const char *event = key;
      GQueue *effects = value;
      g_auto(GVariantBuilder) builder;
      g_autoptr(GPtrArray) names = g_ptr_array_new_full (g_queue_get_length (effects), NULL);

      g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));

      for (GList *link = g_queue_peek_head_link (effects);
           link != NULL;
           link = link->next)
        {
          AttachedEffectInfo *info = link->data;
          g_variant_builder_add (&builder,
                                 "s",
                                 g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (info->server_effect)));
        }

      g_variant_dict_insert_value (&vardict,
                                   event,
                                   g_variant_builder_end (&builder));
    }

  return g_variant_dict_end (&vardict);
}

static void
animations_dbus_server_surface_get_property (GObject    *object,
                                             guint       prop_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  AnimationsDbusServerSurface *server_surface = ANIMATIONS_DBUS_SERVER_SURFACE (object);
  AnimationsDbusServerSurfacePrivate *priv =
    animations_dbus_server_surface_get_instance_private (server_surface);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, priv->connection);
      break;
    case PROP_SERVER:
      g_value_set_object (value, priv->server);
      break;
    case PROP_BRIDGE:
      g_value_set_object (value, priv->bridge);
      break;
    case PROP_TITLE:
      g_value_set_string (value,
                          animations_dbus_server_surface_bridge_get_title (priv->bridge));
      break;
    case PROP_GEOMETRY:
      g_value_set_variant (value,
                           animations_dbus_server_surface_bridge_get_geometry (priv->bridge));
      break;
    case PROP_EFFECTS:
      g_value_set_variant (value,
                           serialize_attached_effects_to_variant (priv->attached_effects_for_events));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
animations_dbus_server_surface_dispose (GObject *object)
{
  AnimationsDbusServerSurface *server_surface = ANIMATIONS_DBUS_SERVER_SURFACE (object);
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);

  priv->server = NULL;
  g_clear_object (&priv->bridge);

  G_OBJECT_CLASS (animations_dbus_server_surface_parent_class)->dispose (object);
}

static void
animations_dbus_server_surface_finalize (GObject *object)
{
  AnimationsDbusServerSurface *server_surface = ANIMATIONS_DBUS_SERVER_SURFACE (object);
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);

  g_clear_pointer (&priv->attached_effects_for_events, g_hash_table_unref);

  G_OBJECT_CLASS (animations_dbus_server_surface_parent_class)->finalize (object);
}

static void
attached_effect_info_queue_free (GQueue *queue)
{
  g_queue_free_full (queue, (GDestroyNotify) attached_effect_info_free);
}

static void
animations_dbus_server_surface_init (AnimationsDbusServerSurface *server_surface)
{
  AnimationsDbusServerSurfacePrivate *priv = animations_dbus_server_surface_get_instance_private (server_surface);

  priv->attached_effects_for_events = g_hash_table_new_full (g_str_hash,
                                                             g_str_equal,
                                                             g_free,
                                                             (GDestroyNotify) attached_effect_info_queue_free);
}

static void
animations_dbus_server_surface_class_init (AnimationsDbusServerSurfaceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = animations_dbus_server_surface_get_property;
  object_class->set_property = animations_dbus_server_surface_set_property;
  object_class->dispose = animations_dbus_server_surface_dispose;
  object_class->finalize = animations_dbus_server_surface_finalize;

  animations_dbus_server_surface_props[PROP_CONNECTION] =
    g_param_spec_object ("connection",
                         "A GDBusConnection",
                         "The GDBusConnection this surface is exported on",
                         G_TYPE_DBUS_CONNECTION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  animations_dbus_server_surface_props[PROP_SERVER] =
    g_param_spec_object ("server",
                         "An AnimationsDbusServer",
                         "The AnimationsDbusServer for this surface",
                         ANIMATIONS_DBUS_TYPE_SERVER,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
  animations_dbus_server_surface_props[PROP_BRIDGE] =
    g_param_spec_object ("bridge",
                         "An AnimationsDbusServerSurfaceBridge",
                         "An AnimationsDbusServerSurfaceBridge used to communicate with the underlying surface",
                         ANIMATIONS_DBUS_TYPE_SERVER_SURFACE_BRIDGE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     N_OWN_PROPS,
                                     animations_dbus_server_surface_props);

  g_object_class_override_property (object_class,
                                    PROP_TITLE,
                                    "title");
  g_object_class_override_property (object_class,
                                    PROP_GEOMETRY,
                                    "geometry");
  g_object_class_override_property (object_class,
                                    PROP_EFFECTS,
                                    "effects");
}

AnimationsDbusServerSurface *
animations_dbus_server_surface_new (GDBusConnection                   *connection,
                                    AnimationsDbusServer              *server,
                                    AnimationsDbusServerSurfaceBridge *bridge)
{
  return g_object_new (ANIMATIONS_DBUS_TYPE_SERVER_SURFACE,
                       "connection", connection,
                       "server", server,
                       "bridge", bridge,
                       NULL);
}
