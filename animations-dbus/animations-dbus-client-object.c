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
#include "animations-dbus-client-object.h"
#include "animations-dbus-client-surface.h"
#include "animations-dbus-errors.h"
#include "animations-dbus-main-context-private.h"
#include "animations-dbus-objects.h"

struct _AnimationsDbusClient
{
  GObject parent_instance;
};

typedef struct _AnimationsDbusClientPrivate
{
  GDBusConnection                      *connection;
  AnimationsDbusConnectionManagerProxy *connection_manager_proxy;
  AnimationsDbusConnectionManagerProxy *animation_manager_proxy;
} AnimationsDbusClientPrivate;

static void animations_dbus_client_async_initable_interface_init (GAsyncInitableIface *iface);

G_DEFINE_TYPE_WITH_CODE (AnimationsDbusClient,
                         animations_dbus_client,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE,
                                                animations_dbus_client_async_initable_interface_init)
                         G_ADD_PRIVATE (AnimationsDbusClient))

enum {
  PROP_0,
  PROP_CONNECTION,
  NPROPS
};

static GParamSpec *animations_dbus_client_properties[NPROPS];

/**
 * animations_dbus_client_list_surfaces_finish:
 * @client: An #AnimationsDbusClient
 * @result: A #GAsyncResult
 * @error: A #GError
 *
 * Finish asynchronously listing surfaces on the server end. There is no
 * guarantee that the surfaces returned exist on the server end by the time
 * they are actually to be used. If a method call is made on a service that
 * does not exist, an error will be returned.
 *
 * Returns: (transfer container) (element-type AnimationsDbusClientSurface):
 *          A #GPtrArray of #AnimationsDbusClientSurface, with each member
 *          connected to a surface on the bus.
 */
GPtrArray *
animations_dbus_client_list_surfaces_finish (AnimationsDbusClient  *client G_GNUC_UNUSED,
                                             GAsyncResult          *result,
                                             GError               **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
} 

typedef struct {
  int           ref_count;
  unsigned int  remaining;
  GPtrArray    *proxies_array;
  GTask        *task;
  GError       *error;
} AllProxiesCountdown;

static AllProxiesCountdown *
all_proxies_countdown_new (unsigned int  n_surfaces,
                           GTask        *task)
{
  AllProxiesCountdown *countdown = g_new0 (AllProxiesCountdown, 1);

  countdown->ref_count = 1;
  countdown->remaining = n_surfaces;
  countdown->proxies_array = g_ptr_array_new_full (n_surfaces, g_object_unref);
  countdown->task = g_object_ref (task);
  countdown->error = NULL;

  return countdown;
}

static AllProxiesCountdown *
all_proxies_countdown_ref (AllProxiesCountdown *countdown)
{
  g_atomic_int_inc (&countdown->ref_count);
  return countdown;
}

static void
all_proxies_countdown_unref (AllProxiesCountdown *countdown)
{
  if (g_atomic_int_dec_and_test (&countdown->ref_count))
    {
      g_clear_pointer (&countdown->proxies_array, g_ptr_array_unref);
      g_clear_object (&countdown->task);
      g_clear_error (&countdown->error);

      g_free (countdown);
    }
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (AllProxiesCountdown, all_proxies_countdown_unref)

static void
on_constructed_animatable_surface_proxy (GObject      *source_object G_GNUC_UNUSED,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
  g_autoptr(AllProxiesCountdown) countdown = user_data;

  /* Note that since the asynchronous call completed, we should still
   * decrement remaining here, even if we will set an error on the AllProxiesCountdown -
   * we need all the async calls to complete before we finally return
   * an error to the caller, since the remaining async calls have a
   * reference on AllProxiesCountdown. */
  unsigned int remaining_after_construction = --countdown->remaining;

  g_autoptr(GError) local_error = NULL;
  g_autoptr(AnimationsDbusAnimatableSurfaceProxy) animatable_surface_proxy =
    ANIMATIONS_DBUS_ANIMATABLE_SURFACE_PROXY (animations_dbus_animatable_surface_proxy_new_finish (result, &local_error));

  if (animatable_surface_proxy == NULL)
    {
      /* Note that on failure we don't necessarily want to set an error
       * on the task itself, since g_task_return_error will cause the
       * task to complete on the next main context iteration, which
       * may happen before all the other async calls have completed.
       *
       * Instead, we set the error on AllProxiesCountdown and wait
       * until they have all completed to return the first error that
       * occurred on the task (all other errors are suppressed).
       *
       * We also don't want to return early here either - we must
       * complete all the asynchronous tasks and run the cleanup
       * code to return.
       */
      if (countdown->error == NULL)
        countdown->error = g_steal_pointer (&local_error);
    }
  else
    {
      g_ptr_array_add (countdown->proxies_array,
                       animations_dbus_client_surface_new_for_proxy (g_steal_pointer (&animatable_surface_proxy)));
    }

   if (remaining_after_construction == 0)
    {
      GTask *task = countdown->task;

      if (countdown->error != NULL)
        {
          g_task_return_error (task, g_steal_pointer (&countdown->error));
          return;
        }

      g_task_return_pointer (task,
                             g_steal_pointer (&countdown->proxies_array),
                             (GDestroyNotify) g_ptr_array_unref);
    }
}

static void
on_animations_dbus_client_list_surfaces (GObject      *source_object,
                                         GAsyncResult *result,
                                         gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  AnimationsDbusClient *client = ANIMATIONS_DBUS_CLIENT (g_task_get_task_data (task));
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);
  g_auto(GStrv) surface_object_paths_array = NULL;
  g_autoptr(GError) local_error = NULL;
  

  if (!animations_dbus_animation_manager_call_list_surfaces_finish (ANIMATIONS_DBUS_ANIMATION_MANAGER (source_object),
                                                                    &surface_object_paths_array,
                                                                    result,
                                                                    &local_error))
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  /* If the surface_object_paths_array is empty, we need to return now, since
   * animations_dbus_animatable_surface_proxy_new will never be called. */
  if (*surface_object_paths_array == NULL)
    {
      g_task_return_pointer (task, g_ptr_array_new (), NULL);
      return;
    }

  /* Now that we have the surface names array, allocate an
   * AllProxiesCountdown and query all the properties of the
   * surfaces that are available. We don't care about ordering. */
  g_autoptr(AllProxiesCountdown) countdown =
    all_proxies_countdown_new (g_strv_length (surface_object_paths_array),
                               task);

  for (const char **surface_object_path_iter = (const char **) surface_object_paths_array;
       *surface_object_path_iter != NULL;
       ++surface_object_path_iter)
    {
      animations_dbus_animatable_surface_proxy_new (priv->connection,
                                                    G_DBUS_PROXY_FLAGS_NONE,
                                                    "com.endlessm.Libanimation",
                                                    *surface_object_path_iter,
                                                    g_task_get_cancellable (task),
                                                    on_constructed_animatable_surface_proxy,
                                                    all_proxies_countdown_ref (countdown));
    }
}

void
animations_dbus_client_list_surfaces_async (AnimationsDbusClient *client,
                                            GCancellable         *cancellable,
                                            GAsyncReadyCallback   callback,
                                            gpointer              user_data)
{
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);
  g_autoptr(GTask) task = g_task_new (client, cancellable, callback, user_data);

  g_task_set_task_data (task, client, NULL);

  animations_dbus_animation_manager_call_list_surfaces (ANIMATIONS_DBUS_ANIMATION_MANAGER (priv->animation_manager_proxy),
                                                        cancellable,
                                                        on_animations_dbus_client_list_surfaces,
                                                        g_steal_pointer (&task));
}

/**
 * animations_dbus_client_list_surfaces:
 * @client: An #AnimationsDbusClientq
 * @error: A #GError
 *
 * List all the available surfaces that can have animations attached to
 * them.
 *
 * Returns: (transfer container) (element-type AnimationsDbusClientSurface):
 *          A #GPtrArray of #AnimationsDbusClientSurface, with each member
 *          connected to a surface on the bus, or %NULL with @error set on
 *          failure.
 */
GPtrArray *
animations_dbus_client_list_surfaces (AnimationsDbusClient  *client,
                                      GError               **error)
{
  g_autoptr(GMainContextPopDefault) context = animations_dbus_main_context_new_default ();
  g_autoptr(GAsyncResult) result = NULL;

  animations_dbus_client_list_surfaces_async (client,
                                              NULL,
                                              animations_dbus_got_async_func_result,
                                              &result);

  while (result == NULL)
    g_main_context_iteration (context, TRUE);

  return animations_dbus_client_list_surfaces_finish (client,
                                                      g_steal_pointer (&result),
                                                      error);
}

/**
 * animations_dbus_client_create_animation_effect_finish:
 * @client: An #AnimationsDbusClient
 * @result: A #GAsyncResult
 * @error: A #GError
 *
 * Finish asynchronously creating an AnimationsDbusClientEffect.
 *
 * Returns: (transfer full): An #AnimationsDbusClientEffect that connects
 *          to an effect created on the bus.
 */
AnimationsDbusClientEffect *
animations_dbus_client_create_animation_effect_finish (AnimationsDbusClient  *client G_GNUC_UNUSED,
                                                       GAsyncResult          *result,
                                                       GError               **error)
{
  g_autoptr(GTask) task = G_TASK (result);
  return g_task_propagate_pointer (task, error);
} 

static void
on_constructed_animation_effect_callback (GObject      *source G_GNUC_UNUSED,
                                          GAsyncResult *result,
                                          gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  g_autoptr(GError) local_error = NULL;
  g_autoptr(AnimationsDbusAnimationEffectProxy) effect_proxy =
    ANIMATIONS_DBUS_ANIMATION_EFFECT_PROXY (animations_dbus_animation_effect_proxy_new_finish (result,
                                                                                               &local_error));

  if (effect_proxy == NULL)
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  g_task_return_pointer (task,
                         animations_dbus_client_effect_new_for_proxy (g_steal_pointer (&effect_proxy)),
                         g_object_unref);
}

static void
on_animations_dbus_client_created_animation_effect (GObject      *source,
                                                    GAsyncResult *result,
                                                    gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  AnimationsDbusClient *client = ANIMATIONS_DBUS_CLIENT (g_task_get_task_data (task));
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);
  g_autofree char *object_path = NULL;
  g_autoptr(GError) local_error = NULL;

  if (!animations_dbus_animation_manager_call_create_animation_effect_finish (ANIMATIONS_DBUS_ANIMATION_MANAGER (source),
                                                                              &object_path,
                                                                              result,
                                                                              &local_error))
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  animations_dbus_animation_effect_proxy_new (priv->connection,
                                              G_DBUS_PROXY_FLAGS_NONE,
                                              "com.endlessm.Libanimation",
                                              object_path,
                                              g_task_get_cancellable (task),
                                              on_constructed_animation_effect_callback,
                                              task);
}

void
animations_dbus_client_create_animation_effect_async (AnimationsDbusClient *client,
                                                      const char           *title,
                                                      const char           *animation,
                                                      GVariant             *settings,
                                                      GCancellable         *cancellable,
                                                      GAsyncReadyCallback   callback,
                                                      gpointer              user_data)
{
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);
  g_autoptr(GTask) task = g_task_new (client, cancellable, callback, user_data);

  g_task_set_task_data (task, client, NULL);

  animations_dbus_animation_manager_call_create_animation_effect (ANIMATIONS_DBUS_ANIMATION_MANAGER (priv->animation_manager_proxy),
                                                                  title,
                                                                  animation,
                                                                  settings,
                                                                  cancellable,
                                                                  on_animations_dbus_client_created_animation_effect,
                                                                  g_steal_pointer (&task));
}

/**
 * animations_dbus_client_create_animation_effect:
 * @client: An #AnimationsDbusClient
 * @title: The title of the effect, for later reference
 * @name: The name of the animation to use. Use animations_dbus_client_surface_list_available_effects
 *        to find out what effects can be created on a surface.
 * @settings: The initial settings.
 * @error: A #GError
 *
 * Create an #AnimationsDbusClientEffect using the animation specified by @name
 * and titled with @title.
 *
 * Returns: (transfer full): An #AnimationsDbusClientEffect that connects
 *          to an effect created on the bus, or %NULL with @error set on failure.
 */
AnimationsDbusClientEffect *
animations_dbus_client_create_animation_effect (AnimationsDbusClient  *client,
                                                const char            *title,
                                                const char            *name,
                                                GVariant              *settings,
                                                GError               **error)
{
  g_autoptr(GMainContextPopDefault) context = animations_dbus_main_context_new_default ();
  g_autoptr(GAsyncResult) result = NULL;

  animations_dbus_client_create_animation_effect_async (client,
                                                        title,
                                                        name,
                                                        settings,
                                                        NULL,
                                                        animations_dbus_got_async_func_result,
                                                        &result);

  while (result == NULL)
    g_main_context_iteration (context, TRUE);

  return animations_dbus_client_create_animation_effect_finish (client,
                                                                g_steal_pointer (&result),
                                                                error);
}

/**
 * animations_dbus_client_new_finish:
 * @source: The object passed to the #GAsyncReadyCallback.
 * @result: A #GAsyncResult.
 * @error: A #GError.
 *
 * Complete the call to animations_dbus_client_new_async.
 *
 * Returns: (transfer full): A new #AnimationsDbusClient or %NULL with
 *          @error set in case of failure.
 */
AnimationsDbusClient *
animations_dbus_client_new_finish (GObject       *source,
                                   GAsyncResult  *result,
                                   GError       **error)
{
  return ANIMATIONS_DBUS_CLIENT (g_async_initable_new_finish (G_ASYNC_INITABLE (source),
                                                              result,
                                                              error));
}

static void
on_created_animation_manager_proxy (GObject      *source G_GNUC_UNUSED,
                                    GAsyncResult *result,
                                    gpointer      user_data)
{
  GTask *task = user_data;
  AnimationsDbusClient *client = g_task_get_task_data (task);
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);
  g_autoptr(GError) local_error = NULL;
  g_autoptr(AnimationsDbusAnimationManagerProxy) animation_manager_proxy =
    ANIMATIONS_DBUS_ANIMATION_MANAGER_PROXY (animations_dbus_animation_manager_proxy_new_finish (result,
                                                                                                 &local_error));

  if (animation_manager_proxy == NULL)
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  priv->animation_manager_proxy = g_steal_pointer (&animation_manager_proxy);

  /* Done with async construction, return TRUE on
   * the async task. */
  g_task_return_boolean (task, TRUE);
}

static void
on_call_register_client_finished (GObject      *source,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  AnimationsDbusClient *client = g_task_get_task_data (task);
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);
  g_autofree char *object_path = NULL;
  g_autoptr(GError) local_error = NULL;

  if (!animations_dbus_connection_manager_call_register_client_finish (ANIMATIONS_DBUS_CONNECTION_MANAGER (source),
                                                                       &object_path,
                                                                       result,
                                                                       &local_error))
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    } 

  animations_dbus_animation_manager_proxy_new (priv->connection,
                                               G_DBUS_PROXY_FLAGS_NONE,
                                               "com.endlessm.Libanimation",
                                               object_path,
                                               g_task_get_cancellable (task),
                                               on_created_animation_manager_proxy,
                                               task);
}

static void
on_created_connection_manager_proxy (GObject      *source G_GNUC_UNUSED,
                                     GAsyncResult *result,
                                     gpointer      user_data)
{
  GTask *task = G_TASK (user_data);
  AnimationsDbusClient *client = g_task_get_task_data (task);
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);
  g_autoptr(GError) local_error = NULL;
  g_autoptr(AnimationsDbusConnectionManagerProxy) connection_manager_proxy =
    ANIMATIONS_DBUS_CONNECTION_MANAGER_PROXY (animations_dbus_connection_manager_proxy_new_finish (result,
                                                                                                   &local_error));

  if (connection_manager_proxy == NULL)
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    }

  priv->connection_manager_proxy = g_steal_pointer (&connection_manager_proxy);

  /* Now that we have a proxy to the connection manager, create an
   * AnimationManager object on the remote end by calling RegisterClient. This
   * will return an object path which we can use the create an
   * AnimationManager proxy. */
  animations_dbus_connection_manager_call_register_client (ANIMATIONS_DBUS_CONNECTION_MANAGER (priv->connection_manager_proxy),
                                                           g_task_get_cancellable (task),
                                                           on_call_register_client_finished,
                                                           task);
}

static inline void
create_animations_dbus_connection_manager_proxy (GDBusConnection *connection,
                                                 GTask           *task)
{
  animations_dbus_connection_manager_proxy_new (connection,
                                                G_DBUS_PROXY_FLAGS_NONE,
                                                "com.endlessm.Libanimation",
                                                "/com/endlessm/Libanimation/ConnectionManager",
                                                g_task_get_cancellable (task),
                                                on_created_connection_manager_proxy,
                                                task);
}

static void
on_got_session_bus (GObject      *source G_GNUC_UNUSED,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  GTask *task = user_data;
  AnimationsDbusClient *client = g_task_get_task_data (task);
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);
  g_autoptr(GError) local_error = NULL;
  g_autoptr(GDBusConnection) connection = g_bus_get_finish (result, &local_error);

  if (connection == NULL)
    {
      g_task_return_error (task, g_steal_pointer (&local_error));
      return;
    } 

  priv->connection = g_steal_pointer (&connection);

  /* Now that we have the session bus, create a proxy for
   * the com.endlessm.Libanimation.ConnectionManager interface
   * com.endlessm.Libanimation:/com/endlessm/Libanimation/ConnectionManager */
  create_animations_dbus_connection_manager_proxy (priv->connection, task);
}

static gboolean
animations_dbus_client_init_finish (GAsyncInitable  *initable,
                                    GAsyncResult    *result,
                                    GError         **error)
{
  GTask *task = G_TASK (result);
  g_return_val_if_fail (g_task_is_valid (task, initable), FALSE);

  return g_task_propagate_boolean (task, error);
}

static void
animations_dbus_client_init_async (GAsyncInitable      *initable,
                                   int                  io_priority G_GNUC_UNUSED,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  AnimationsDbusClient *client = ANIMATIONS_DBUS_CLIENT (initable);
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);
  GTask *task = g_task_new (initable, cancellable, callback, user_data);

  /* Async initialization does not own the client, the caller
   * owns it. */
  g_task_set_task_data (task, client, NULL);

  /* Already initialized */
  if (priv->animation_manager_proxy != NULL)
    {
      g_task_return_boolean (task, TRUE);
      return;
    }

  /* The caller provided us with a connection on construction. This
   * means that we do not have to get the session bus singleton
   * and can continue with calling
   * create_animations_dbus_connection_manager_proxy */
  if (priv->connection != NULL)
    {
      create_animations_dbus_connection_manager_proxy (priv->connection,
                                                       task);
      return;
    }

  g_bus_get (G_BUS_TYPE_SESSION,
             cancellable,
             on_got_session_bus,
             task);
}

static void
animations_dbus_client_async_initable_interface_init (GAsyncInitableIface *iface)
{
  iface->init_async = animations_dbus_client_init_async;
  iface->init_finish = animations_dbus_client_init_finish;
}

static void
animations_dbus_client_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  AnimationsDbusClient *client = ANIMATIONS_DBUS_CLIENT (object);
  AnimationsDbusClientPrivate *priv =
    animations_dbus_client_get_instance_private (client);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      priv->connection = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
animations_dbus_client_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  AnimationsDbusClient *client = ANIMATIONS_DBUS_CLIENT (object);
  AnimationsDbusClientPrivate *priv =
    animations_dbus_client_get_instance_private (client);

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
animations_dbus_client_dispose (GObject *object)
{
  AnimationsDbusClient *client = ANIMATIONS_DBUS_CLIENT (object);
  AnimationsDbusClientPrivate *priv = animations_dbus_client_get_instance_private (client);

  g_clear_object (&priv->animation_manager_proxy);
  g_clear_object (&priv->connection_manager_proxy);
  g_clear_object (&priv->connection);

  G_OBJECT_CLASS (animations_dbus_client_parent_class)->dispose (object);
}

static void
animations_dbus_client_init (AnimationsDbusClient *client G_GNUC_UNUSED)
{
}

static void
animations_dbus_client_class_init (AnimationsDbusClientClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = animations_dbus_client_dispose;
  object_class->set_property = animations_dbus_client_set_property;
  object_class->get_property = animations_dbus_client_get_property;

  animations_dbus_client_properties[PROP_CONNECTION] =
    g_param_spec_object ("connection",
                         "A GDBusConnection",
                         "The connection connecting this client to the session bus",
                         G_TYPE_DBUS_CONNECTION,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class,
                                     NPROPS,
                                     animations_dbus_client_properties);
}

void
animations_dbus_client_new_async (GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  g_async_initable_new_async (ANIMATIONS_DBUS_TYPE_CLIENT,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              NULL);
}

void
animations_dbus_client_new_with_connection_async (GDBusConnection     *connection,
                                                  GCancellable        *cancellable,
                                                  GAsyncReadyCallback  callback,
                                                  gpointer             user_data)
{
  g_async_initable_new_async (ANIMATIONS_DBUS_TYPE_CLIENT,
                              G_PRIORITY_DEFAULT,
                              cancellable,
                              callback,
                              user_data,
                              "connection", connection,
                              NULL);
}

/**
 * animations_dbus_client_new:
 * @error: A #GError.
 *
 * Create a new AnimationsDbusClient by connecting to the session bus
 * and registering the connection with the owner of com.endlessm.Libanimation.
 *
 * Returns: (transfer full): A new #AnimationsDbusClient or %NULL with
 *          @error set in case of failure.
 */
AnimationsDbusClient *
animations_dbus_client_new (GError **error)
{
  g_autoptr(GMainContextPopDefault) context = animations_dbus_main_context_new_default ();
  g_autoptr(GAsyncResult) result = NULL;
  g_autoptr(AnimationsDbusClient) client = g_object_new (ANIMATIONS_DBUS_TYPE_CLIENT, NULL);

  g_async_initable_init_async (G_ASYNC_INITABLE (client),
                               G_PRIORITY_DEFAULT,
                               NULL,
                               animations_dbus_got_async_func_result,
                               &result);

  while (result == NULL)
    g_main_context_iteration (context, TRUE);

  if (!g_async_initable_init_finish (G_ASYNC_INITABLE (client),
                                     g_steal_pointer (&result),
                                     error))
    return NULL;

  return g_steal_pointer (&client);
}

/**
 * animations_dbus_client_new_with_connection:
 * @connection: A #GDBusConnection.
 * @error: A #GError.
 *
 * Create a new AnimationsDbusClient by using the existing connection
 * and registering the connection with the owner of com.endlessm.Libanimation.
 *
 * Returns: (transfer full): A new #AnimationsDbusClient or %NULL with
 *          @error set in case of failure.
 */
AnimationsDbusClient *
animations_dbus_client_new_with_connection (GDBusConnection  *connection,
                                            GError          **error)
{
  g_autoptr(GMainContextPopDefault) context = animations_dbus_main_context_new_default ();
  g_autoptr(GAsyncResult) result = NULL;
  g_autoptr(AnimationsDbusClient) client =
    g_object_new (ANIMATIONS_DBUS_TYPE_CLIENT,
                  "connection", connection,
                  NULL);

  g_async_initable_init_async (G_ASYNC_INITABLE (client),
                               G_PRIORITY_DEFAULT,
                               NULL,
                               animations_dbus_got_async_func_result,
                               &result);

  while (result == NULL)
    g_main_context_iteration (context, TRUE);

  if (!g_async_initable_init_finish (G_ASYNC_INITABLE (client),
                                     g_steal_pointer (&result),
                                     error))
    return NULL;

  return g_steal_pointer (&client);
}
