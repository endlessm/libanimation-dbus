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
#include "animations-dbus-server-object.h"
#include "animations-dbus-server-animation-manager.h"
#include "animations-dbus-server-effect-factory-interface.h"
#include "animations-dbus-server-surface.h"

struct _AnimationsDbusServer
{
  GObject parent_instance;
};

typedef struct _AnimationsDbusServerPrivate
{
  GDBusConnection                         *connection;  /* (owned) */
  guint                                    name_id;
  AnimationsDbusConnectionManagerSkeleton *connection_manager_skeleton;

  AnimationsDbusServerEffectFactory       *effect_factory;

  /* One AnimationManager per client connection */
  GHashTable *animation_manager_ids; /* (key-type: utf8) (value-type: guint) */
  GHashTable *animation_managers;  /* (key-type: guint) (value-type: AnimationsDbusAnimationManager) */
  guint       animation_manager_serial;

  /* When a client calls RegisterClient and we create an
   * AnimationManager for them, we also watch their owned
   * bus name to see when it disappears. We can then tear
   * down the corresponding AnimationManager for that bus
   * name, ensuring a clean state when the client exits. */
  GHashTable  *client_name_watches;

  /* One AnimatableSurface per surface that is animatable.
   *
   * Add surfaces with animations_dbus_server_register_surface()
   * and remove surfaces with animations_dbus_server_unregister_surface(). */
  GPtrArray *animatable_surfaces; /* (element-type: AnimationsDbusServerSurface) */
  guint      animatable_surface_serial;
} AnimationsDbusServerPrivate;

enum {
  PROP_0,
  PROP_CONNECTION,
  PROP_EFFECT_FACTORY,
  NPROPS
};

static GParamSpec *animations_dbus_server_props[NPROPS];

enum {
  SIGNAL_CLIENT_CONNECTED,
  SIGNAL_CLIENT_DISCONNECTED,
  NSIGNALS
};

static unsigned int animations_dbus_server_signals[NSIGNALS];

static void animations_dbus_server_async_initable_interface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (AnimationsDbusServer,
                         animations_dbus_server,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE,
                                                animations_dbus_server_async_initable_interface_init)
                         G_ADD_PRIVATE (AnimationsDbusServer))

/**
 * animations_dbus_server_lookup_animation_effect_by_ids:
 * @server: A #AnimationsDbusServer
 * @animation_manager_id: The ID of the animation manager that the effect is on.
 * @animation_effect_id: The ID of the effect.
 * @error: A #GError
 *
 * Get the #AnimationsDbusServerEffect for the given @animation_manager_id and
 * @animation_effect_id, or %NULL with @error set if either one does not exist.
 *
 * Returns: (transfer none): An #AnimationsDbusServerEffect if one exists for the given
 *                           @animation_manager_id and @animation_effect_id or %NULL
 *                           otherwise.
 */
AnimationsDbusServerEffect *
animations_dbus_server_lookup_animation_effect_by_ids (AnimationsDbusServer  *server,
                                                       unsigned int           animation_manager_id,
                                                       unsigned int           animation_effect_id,
                                                       GError               **error)
{
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);
  const char *client_name = g_hash_table_lookup (priv->animation_manager_ids,
                                                 GUINT_TO_POINTER (animation_manager_id));

  if (client_name == NULL)
    {
      /* We use ANIMATIONS_DBUS_ERROR_NO_SUCH_EFFECT since that is
       * basically mean to signify a bad object path when looking
       * up an effect */
      g_set_error (error,
                   ANIMATIONS_DBUS_ERROR,
                   ANIMATIONS_DBUS_ERROR_NO_SUCH_ANIMATION,
                   "No animation manager with id %u",
                   animation_manager_id);
      return NULL;
    }

  AnimationsDbusServerAnimationManager *animation_manager =
    g_hash_table_lookup (priv->animation_managers,
                         client_name);

  g_assert (animation_manager != NULL);

  return animations_dbus_server_animation_manager_lookup_effect_by_id (animation_manager,
                                                                       animation_effect_id,
                                                                       error);
}

/**
 * animations_dbus_server_list_surfaces:
 * @server: A #AnimationsDbusServer
 *
 * Get a #GPtrArray of surfaces that this #AnimationsDbusServer is tracking.
 *
 * Returns: (transfer none) (element-type AnimationsDbusServerSurface): A #GPtrArray
 * of #AnimationsDbusServerSurface.
 */
GPtrArray *
animations_dbus_server_list_surfaces (AnimationsDbusServer *server)
{
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);

  return priv->animatable_surfaces;
}

#define ANIMATIONS_DBUS_ANIMATABLE_SURFACE_OBJECT_PATH_TEMPLATE "/com/endlessm/Libanimation/AnimatableSurface/%u"

/**
 * animations_dbus_server_register_surface:
 * @server: A #AnimationsDbusServer.
 * @bridge: An #AnimationsDbusServerSurfaceBridge.
 * @error: A #GError
 *
 * Register a new surface, using @bridge to register effects and get properties
 * on the underlying surface.
 *
 * Returns: (transfer full): An #AnimationsDbusServerSurface for the @title and
 *          @geometry of the surface, or %NULL with @error set in case the surface
 *          is not there.
 */
AnimationsDbusServerSurface *
animations_dbus_server_register_surface (AnimationsDbusServer               *server,
                                         AnimationsDbusServerSurfaceBridge  *bridge,
                                         GError                            **error)
{
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);
  g_autoptr(AnimationsDbusServerSurface) server_surface =
    animations_dbus_server_surface_new (priv->connection,
                                        server,
                                        bridge);
  unsigned int allocated_id = priv->animatable_surface_serial++;
  g_autofree char *object_path = g_strdup_printf (ANIMATIONS_DBUS_ANIMATABLE_SURFACE_OBJECT_PATH_TEMPLATE, allocated_id);

  if (!animations_dbus_server_surface_export (server_surface,
                                              object_path,
                                              error))
    return NULL;

  g_ptr_array_add (priv->animatable_surfaces, g_object_ref (server_surface));
  return g_steal_pointer (&server_surface);
}

/**
 * animations_dbus_server_unregister_surface:
 * @server: A #AnimationsDbusServer
 * @server_surface: The surface to unregister.
 * @error: A #GError
 *
 * Unregister and unexport the given surface.
 *
 * Returns: %TRUE if the surface was unregistered and unexported correctly, %FALSE
 *          with @error set otherwise.
 */
gboolean
animations_dbus_server_unregister_surface (AnimationsDbusServer         *server,
                                           AnimationsDbusServerSurface  *server_surface,
                                           GError                      **error)
{
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);

  if (!g_ptr_array_remove_fast (priv->animatable_surfaces, server_surface))
    {
      g_set_error (error,
                   ANIMATIONS_DBUS_ERROR,
                   ANIMATIONS_DBUS_ERROR_SERVER_SURFACE_NOT_FOUND,
                   "Server surface %p was not found",
                   (gpointer) server_surface);
      return FALSE;
    }

  animations_dbus_server_surface_unexport (server_surface);
  return TRUE;
}

#define LIBANIMATION_ANIMATION_MANAGER_OBJECT_PATH_TEMPLATE "/com/endlessm/Libanimation/AnimationManager/%u"

/**
 * animations_dbus_server_create_animation_manager:
 * @server: An #AnimationsDbusServer.
 * @error: A #GError.
 *
 * Create a #AnimationsDbusServerAnimationManager for use by the
 * server side. The animation manager will appear on the bus but
 * will be managed by the caller (for instance, there is no bus name
 * that is being watched for the animation manager to be destroyed).
 *
 * Returns: (transfer full): A #AnimationsDbusServerAnimationManager
 * or %NULL with @error set in case of an error.
 */
AnimationsDbusServerAnimationManager *
animations_dbus_server_create_animation_manager (AnimationsDbusServer  *server,
                                                 GError               **error)
{
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);
  g_autoptr(AnimationsDbusServerAnimationManager) server_animation_manager =
    animations_dbus_server_animation_manager_new (priv->connection,
                                                  server,
                                                  priv->effect_factory);

  /* Need to be careful to use preincrement here as we will be
   * doing a reverse lookup of this later and we can't have 0
   * as 0 == NULL, which would indicate that an id was not found
   * for a given client name */
  unsigned int allocated_id = ++priv->animation_manager_serial;
  g_autofree char *object_path = g_strdup_printf (LIBANIMATION_ANIMATION_MANAGER_OBJECT_PATH_TEMPLATE,
                                                  allocated_id);

  if (!animations_dbus_server_animation_manager_export (server_animation_manager,
                                                        object_path,
                                                        error))
    return NULL;

  /* Because the caller could be a user of the server library and not
   * the server library itself
   * (from on_animation_connection_manager_register_client), we don't
   * register the newly created AnimationsDbusServerAnimationManager in
   * the hash table, since it is up to the caller to keep track of its
   * lifetime. */
  return g_steal_pointer (&server_animation_manager);
}

static gboolean
remove_if_value_strequal (gpointer key G_GNUC_UNUSED,
                          gpointer value,
                          gpointer user_data)
{
  return g_str_equal ((const char *) value, (const char *) user_data);
}

static void
on_animation_manager_owner_name_lost (GDBusConnection *connection G_GNUC_UNUSED,
                                      const char      *name,
                                      gpointer         user_data)
{
  AnimationsDbusServer *server = user_data;
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);
  gpointer watch_id_ptr = NULL;

  if (g_hash_table_lookup_extended (priv->client_name_watches,
                                    name,
                                    NULL,
                                    (gpointer *) &watch_id_ptr))
    {
      unsigned int watch_id = GPOINTER_TO_UINT (watch_id_ptr);

      g_bus_unwatch_name (watch_id);
      g_hash_table_remove (priv->client_name_watches, name);
      g_hash_table_remove (priv->animation_managers, name);

      /* Need to do an O(N) lookup of ids names to remove the
       * corresponding entry in the id table. We could also
       * use an additional hash table here to avoid this cost
       * but this seems like a simpler solution. */
      unsigned int n_removed = g_hash_table_foreach_remove (priv->animation_manager_ids,
                                                            remove_if_value_strequal,
                                                            (gpointer) name);

      g_assert (n_removed > 0);

      g_signal_emit (server,
                     animations_dbus_server_signals[SIGNAL_CLIENT_DISCONNECTED],
                     0,
                     name);
    }
}

static gboolean
on_animation_connection_manager_register_client (AnimationsDbusConnectionManager *connection_manager,
                                                 GDBusMethodInvocation           *invocation,
                                                 gpointer                         user_data)
{
  AnimationsDbusServer *server = user_data;
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);
  const char *sender = g_dbus_method_invocation_get_sender (invocation);
  g_autoptr(GError) local_error = NULL;

  if (g_hash_table_contains (priv->animation_managers, sender))
    {
      g_autofree char *message = g_strdup_printf ("Name '%s' already has an AnimationManager "
                                                  "registered", sender);
      g_dbus_method_invocation_return_error_literal (invocation,
                                                     ANIMATIONS_DBUS_ERROR,
                                                     ANIMATIONS_DBUS_ERROR_NAME_ALREADY_REGISTERED,
                                                     message);
      return TRUE;
    }

  g_autoptr(AnimationsDbusServerAnimationManager) server_animation_manager =
    animations_dbus_server_create_animation_manager (server, &local_error);

  if (server_animation_manager == NULL)
    {
      g_autofree char *message = g_strdup_printf ("Could not register AnimationManager "
                                                  "for name '%s': %s",
                                                  sender,
                                                  local_error->message);
      g_dbus_method_invocation_return_error_literal (invocation,
                                                     ANIMATIONS_DBUS_ERROR,
                                                     ANIMATIONS_DBUS_ERROR_INTERNAL_ERROR,
                                                     message);
      return TRUE;
    }

  /* Watch the name on the connection. If the name disappears, we can remove
   * the animation manager and drop all of its associated effects. */
  guint name_watch_id = g_bus_watch_name_on_connection (priv->connection,
                                                        sender,
                                                        G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                        NULL,
                                                        on_animation_manager_owner_name_lost,
                                                        server,
                                                        NULL);
  g_hash_table_insert (priv->client_name_watches,
                       g_strdup (sender),
                       GINT_TO_POINTER (name_watch_id));
  g_hash_table_insert (priv->animation_manager_ids,
                       GINT_TO_POINTER (priv->animation_manager_serial),
                       g_strdup (sender));
  g_hash_table_insert (priv->animation_managers,
                       g_strdup (sender),
                       g_object_ref (server_animation_manager));

  g_message ("Registering client '%s'", sender);

  g_signal_emit (server,
                 animations_dbus_server_signals[SIGNAL_CLIENT_CONNECTED],
                 0,
                 sender);

  animations_dbus_connection_manager_complete_register_client (connection_manager,
                                                               invocation,
                                                               g_dbus_interface_skeleton_get_object_path (G_DBUS_INTERFACE_SKELETON (server_animation_manager)));
  return TRUE;
}

#define LIBANIMATION_DBUS_NAME "com.endlessm.Libanimation"
#define LIBANIMATION_CONNECTION_MANAGER_OBJECT_PATH "/com/endlessm/Libanimation/ConnectionManager"

static void
on_got_session_bus_name (GDBusConnection *connection,
                         const char      *name G_GNUC_UNUSED,
                         gpointer         user_data)
{
  GTask *task = user_data;
  g_autoptr(GError) local_error = NULL;
  AnimationsDbusServer *server = g_task_get_task_data (task);
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);

  /* Now that we have the name, we can continue with constructing
   * the connection manager. */
  g_autoptr(AnimationsDbusConnectionManagerSkeleton) connection_manager_skeleton =
    ANIMATIONS_DBUS_CONNECTION_MANAGER_SKELETON (animations_dbus_connection_manager_skeleton_new ());

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (connection_manager_skeleton),
                                         connection,
                                         LIBANIMATION_CONNECTION_MANAGER_OBJECT_PATH,
                                         &local_error))
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  g_signal_connect_object (connection_manager_skeleton,
                           "handle-register-client",
                           G_CALLBACK (on_animation_connection_manager_register_client),
                           server,
                           G_CONNECT_AFTER);

  priv->connection_manager_skeleton = g_steal_pointer (&connection_manager_skeleton);
  g_task_return_boolean (task, TRUE);
}

static void
on_lost_session_bus_name (GDBusConnection *connection G_GNUC_UNUSED,
                          const char      *name G_GNUC_UNUSED,
                          gpointer         user_data)
{
  GTask *task = user_data;

  /* Two possibilities - either the task has already completed
   * and we lost the name for some other reason (maybe someone
   * replaced us) or the task has not completed and we need to
   * return an error to the caller to signify that async object
   * initialization was not possible. */
  if (g_task_get_completed (task))
    {
      /* Do nothing - the task will be unreferenced and
       * cleaned up - someone else has replaced us. */
      return;
    }

  /* Need to return an error to the caller */
  g_task_return_new_error (task,
                           ANIMATIONS_DBUS_ERROR,
                           ANIMATIONS_DBUS_ERROR_CONNECTION_FAILED,
                           "Couldn't get ownership of bus name " LIBANIMATION_DBUS_NAME);
}

static inline unsigned int
attempt_to_own_session_bus_name (GDBusConnection *connection,
                                 GTask           *task  /* (transfer full) */)
{
  return g_bus_own_name_on_connection (connection,
                                       LIBANIMATION_DBUS_NAME,
                                       G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE,
                                       on_got_session_bus_name,
                                       on_lost_session_bus_name,
                                       g_steal_pointer (&task),
                                       g_object_unref);
}

static void
on_got_session_bus_connection (GObject      *object G_GNUC_UNUSED,
                               GAsyncResult *result,
                               gpointer      user_data)
{
  g_autoptr(GError) local_error = NULL;
  g_autoptr(GDBusConnection) connection = g_bus_get_finish (result, &local_error);
  g_autoptr(GTask) task = user_data;
  AnimationsDbusServer *server = g_task_get_task_data (task);
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);

  if (connection == NULL)
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  priv->connection = g_steal_pointer (&connection);

  /* Now that we have the connection, own the bus name on
   * behalf of the caller. */
  priv->name_id = attempt_to_own_session_bus_name (priv->connection, g_steal_pointer (&task));
}

static gboolean
animations_dbus_server_init_finish (GAsyncInitable  *initable,
                                    GAsyncResult    *result,
                                    GError         **error)
{
  g_return_val_if_fail (g_task_is_valid (result, initable), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
animations_dbus_server_init_async (GAsyncInitable      *initable,
                                   int                  io_priority G_GNUC_UNUSED,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  AnimationsDbusServer *server = ANIMATIONS_DBUS_SERVER (initable);
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);
  g_autoptr(GTask) task = g_task_new (initable, cancellable, callback, user_data);

  g_task_set_task_data (task, server, NULL);

  if (priv->connection_manager_skeleton != NULL)
    {
      g_task_return_boolean (task, TRUE);
      return;
    }

  /* The caller already passed us a connection that we can use,
   * continue on to calling attempt_to_own_session_bus_name */
  if (priv->connection != NULL)
    {
      priv->name_id = attempt_to_own_session_bus_name (priv->connection,
                                                       g_steal_pointer (&task));
      return;
    }

  g_bus_get (G_BUS_TYPE_SESSION,
             cancellable,
             on_got_session_bus_connection,
             g_steal_pointer (&task));
}

static void
animations_dbus_server_async_initable_interface_init (GAsyncInitableIface *iface)
{
  iface->init_async = animations_dbus_server_init_async;
  iface->init_finish = animations_dbus_server_init_finish;
}

static void
animations_dbus_server_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  AnimationsDbusServer *server = ANIMATIONS_DBUS_SERVER (object);
  AnimationsDbusServerPrivate *priv =
    animations_dbus_server_get_instance_private (server);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      priv->connection = g_value_dup_object (value);
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
animations_dbus_server_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  AnimationsDbusServer *server = ANIMATIONS_DBUS_SERVER (object);
  AnimationsDbusServerPrivate *priv =
    animations_dbus_server_get_instance_private (server);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, priv->connection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
animations_dbus_server_dispose (GObject *object)
{
  AnimationsDbusServer *server = ANIMATIONS_DBUS_SERVER (object);
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);

  g_clear_object (&priv->connection);
  g_clear_object (&priv->connection_manager_skeleton);
  g_clear_object (&priv->effect_factory);

  g_clear_pointer (&priv->animation_managers, g_hash_table_unref);
  g_clear_pointer (&priv->animatable_surfaces, g_ptr_array_unref);

  G_OBJECT_CLASS (animations_dbus_server_parent_class)->dispose (object);
}

static void
animations_dbus_server_finalize (GObject *object)
{
  AnimationsDbusServer *server = ANIMATIONS_DBUS_SERVER (object);
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);

  g_clear_pointer (&priv->client_name_watches, g_hash_table_unref);

  if (priv->name_id != 0)
    {
      g_bus_unown_name (priv->name_id);
      priv->name_id = 0;
    }

  G_OBJECT_CLASS (animations_dbus_server_parent_class)->finalize (object);
}

static void
animations_dbus_server_init (AnimationsDbusServer *server)
{
  AnimationsDbusServerPrivate *priv = animations_dbus_server_get_instance_private (server);

  priv->animatable_surfaces = g_ptr_array_new_with_free_func (g_object_unref);
  priv->animation_managers = g_hash_table_new_full (g_str_hash,
                                                    g_str_equal,
                                                    g_free,
                                                    g_object_unref);
  priv->animation_manager_ids = g_hash_table_new_full (g_direct_hash,
                                                       g_direct_equal,
                                                       NULL,
                                                       g_free);
  priv->client_name_watches = g_hash_table_new_full (g_str_hash,
                                                     g_str_equal,
                                                     g_free,
                                                     NULL);
}

static void
animations_dbus_server_class_init (AnimationsDbusServerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = animations_dbus_server_get_property;
  object_class->set_property = animations_dbus_server_set_property;
  object_class->dispose = animations_dbus_server_dispose;
  object_class->finalize = animations_dbus_server_finalize;

  animations_dbus_server_props[PROP_CONNECTION] =
    g_param_spec_object ("connection",
                         "A GDBusConnection",
                         "The GDBusConnection connecting this server to the session bus",
                         G_TYPE_DBUS_CONNECTION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  animations_dbus_server_props[PROP_EFFECT_FACTORY] =
    g_param_spec_object ("effect-factory",
                         "An AnimationsDbusServerEffectFactory",
                         "The AnimationsDbusServerEffectFactory that creates the animations",
                         ANIMATIONS_DBUS_TYPE_SERVER_EFFECT_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     NPROPS,
                                     animations_dbus_server_props);

  animations_dbus_server_signals[SIGNAL_CLIENT_CONNECTED] =
    g_signal_new ("client-connected",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);

  animations_dbus_server_signals[SIGNAL_CLIENT_DISCONNECTED] =
    g_signal_new ("client-disconnected",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);
}

/**
 * animations_dbus_server_new_finish:
 * @source: The object passed to the #GAsyncReadyCallback.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Complete the call to animations_dbus_server_new_async.
 *
 * Returns: (transfer full): A new #AnimationsDbusServer or %NULL with
 *          @error set in case of failure.
 */
AnimationsDbusServer *
animations_dbus_server_new_finish (GObject       *source,
                                   GAsyncResult  *result,
                                   GError       **error)
{
  return ANIMATIONS_DBUS_SERVER (g_async_initable_new_finish (G_ASYNC_INITABLE (source),
                                                              result,
                                                              error));
}

void
animations_dbus_server_new_async (AnimationsDbusServerEffectFactory *factory,
                                  GCancellable                      *cancellable,
                                  GAsyncReadyCallback                callback,
                                  gpointer                           user_data)
{
  g_async_initable_new_async (ANIMATIONS_DBUS_TYPE_SERVER,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "effect-factory", factory,
                              NULL);
}

void
animations_dbus_server_new_with_connection_async (AnimationsDbusServerEffectFactory *factory,
                                                  GDBusConnection                   *connection,
                                                  GCancellable                      *cancellable,
                                                  GAsyncReadyCallback                callback,
                                                  gpointer                           user_data)
{
  g_async_initable_new_async (ANIMATIONS_DBUS_TYPE_SERVER,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "connection", connection,
                              "effect-factory", factory,
                              NULL);
}
