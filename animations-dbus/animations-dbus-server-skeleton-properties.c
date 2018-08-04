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

#include <glib.h>

#include "animations-dbus-errors.h"
#include "animations-dbus-server-skeleton-properties.h"

static gpointer
alloc_gtype (GType type)
{
  GType *ptr = g_new0 (GType, 1);

  *ptr = type;
  return ptr;
}

static gpointer
value_type_to_variant_type_ht (gpointer data G_GNUC_UNUSED)
{
  GHashTable *ht = g_hash_table_new_full (g_int_hash, g_int_equal, g_free, NULL);

  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_STRING),
                       (gpointer) G_VARIANT_TYPE_STRING);
  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_STRV),
                       (gpointer) G_VARIANT_TYPE_STRING_ARRAY);
  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_BOOLEAN),
                       (gpointer) G_VARIANT_TYPE_BOOLEAN);
  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_UCHAR),
                       (gpointer) G_VARIANT_TYPE_BYTE);
  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_INT),
                       (gpointer) G_VARIANT_TYPE_INT32);
  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_UINT),
                       (gpointer) G_VARIANT_TYPE_UINT32);
  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_INT64),
                       (gpointer) G_VARIANT_TYPE_INT64);
  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_UINT64),
                       (gpointer) G_VARIANT_TYPE_UINT64);
  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_DOUBLE),
                       (gpointer) G_VARIANT_TYPE_DOUBLE);
  g_hash_table_insert (ht,
                       alloc_gtype (G_TYPE_VARIANT),
                       (gpointer) G_VARIANT_TYPE_VARIANT);

  return ht;
}

static const GVariantType *
value_type_to_variant_type (GType type)
{
  static GOnce value_type_to_variant_type_once;

  g_once (&value_type_to_variant_type_once, value_type_to_variant_type_ht, NULL);

  const GVariantType *variant_type =
    g_hash_table_lookup (value_type_to_variant_type_once.retval, &type);

  if (variant_type == NULL)
    g_warning ("Cannot convert from GType %lu to GVariantType", type);

  return variant_type;
}

static GVariantDict *
serialize_properties_to_variant_dict (GObject *object)
{
  unsigned int n_pspecs = 0;
  GParamSpec **pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (object), &n_pspecs);
  g_autoptr(GVariantDict) vardict = g_variant_dict_new (NULL);

  g_variant_dict_init (vardict, NULL);

  for (unsigned int i = 0; i < n_pspecs; ++i)
    {
      g_auto(GValue) value = G_VALUE_INIT;

      g_value_init (&value, pspecs[i]->value_type);
      g_object_get_property (object, pspecs[i]->name, &value);

      /* Non-floating reference */
      g_autoptr(GVariant) variant =
        g_dbus_gvalue_to_gvariant (&value, value_type_to_variant_type (pspecs[i]->value_type));
      g_variant_dict_insert_value (vardict, pspecs[i]->name, variant);
    }

  g_free (pspecs);
  return g_steal_pointer (&vardict);
}

GVariant *
animations_dbus_serialize_properties_to_variant (GObject *object)
{
  g_autoptr(GVariantDict) vardict = serialize_properties_to_variant_dict (object);

  return g_variant_dict_end (vardict);
}

static void
get_range_variants_from_pspec (GParamSpec *pspec,
                               GVariant   **out_min_variant,
                               GVariant   **out_max_variant)
{
  g_assert (out_min_variant != NULL);
  g_assert (out_max_variant != NULL);

  *out_min_variant = NULL;
  *out_max_variant = NULL;

  switch (pspec->value_type)
    {
    case G_TYPE_INT:
      {
        GParamSpecInt *pspec_int = G_PARAM_SPEC_INT (pspec);
        *out_min_variant = g_variant_new_int32 (pspec_int->minimum);
        *out_max_variant = g_variant_new_int32 (pspec_int->maximum);
      }
      break;
    case G_TYPE_UINT:
      {
        GParamSpecUInt *pspec_uint = G_PARAM_SPEC_UINT (pspec);
        *out_min_variant = g_variant_new_uint32 (pspec_uint->minimum);
        *out_max_variant = g_variant_new_uint32 (pspec_uint->maximum);
      }
      break;
    case G_TYPE_LONG:
      {
        GParamSpecLong *pspec_long = G_PARAM_SPEC_LONG (pspec);
        *out_min_variant = g_variant_new_int64 (pspec_long->minimum);
        *out_max_variant = g_variant_new_int64 (pspec_long->maximum);
      }
      break;
    case G_TYPE_ULONG:
      {
        GParamSpecULong *pspec_ulong = G_PARAM_SPEC_ULONG (pspec);
        *out_min_variant = g_variant_new_uint64 (pspec_ulong->minimum);
        *out_max_variant = g_variant_new_uint64 (pspec_ulong->maximum);
      }
      break;
    case G_TYPE_FLOAT:
      {
        GParamSpecFloat *pspec_float = G_PARAM_SPEC_FLOAT (pspec);
        *out_min_variant = g_variant_new_double (pspec_float->minimum);
        *out_max_variant = g_variant_new_double (pspec_float->maximum);
      }
      break;
    case G_TYPE_DOUBLE:
      {
        GParamSpecDouble *pspec_double = G_PARAM_SPEC_DOUBLE (pspec);
        *out_min_variant = g_variant_new_double (pspec_double->minimum);
        *out_max_variant = g_variant_new_double (pspec_double->maximum);
      }
      break;
    }
}

GVariant *
animations_dbus_serialize_pspecs_to_variant (GObject *object)
{
  unsigned int n_pspecs = 0;
  GParamSpec **pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (object), &n_pspecs);
  g_auto(GVariantDict) vardict;

  g_variant_dict_init (&vardict, NULL);

  for (unsigned int i = 0; i < n_pspecs; ++i)
    {
      const GVariantType *prop_variant_type = value_type_to_variant_type (pspecs[i]->value_type);
      GVariantDict prop_dict;

      g_variant_dict_init (&prop_dict, NULL);
      g_variant_dict_insert (&prop_dict,
                             "type",
                             "s",
                             g_variant_type_peek_string (prop_variant_type));

      const GValue *default_value =
        g_param_spec_get_default_value (pspecs[i]);
      /* Non-floating reference */
      g_autoptr(GVariant) default_value_variant =
        g_dbus_gvalue_to_gvariant (default_value, prop_variant_type);

      g_variant_dict_insert_value (&prop_dict,
                                   "default",
                                   default_value_variant);

      g_autoptr(GVariant) min_variant = NULL;
      g_autoptr(GVariant) max_variant = NULL;
      get_range_variants_from_pspec (pspecs[i], &min_variant, &max_variant);

      /* Need to sink the floating reference on the outparam variants
       * as they are owned by autoptr */
      if (min_variant != NULL)
        {
          g_variant_ref_sink (min_variant);
          g_variant_dict_insert_value (&prop_dict, "min", min_variant);
        }

      if (max_variant != NULL)
        {
          g_variant_ref_sink (max_variant);
          g_variant_dict_insert_value (&prop_dict, "max", max_variant);
        }

      g_variant_dict_insert_value (&vardict,
                                   pspecs[i]->name,
                                   g_variant_dict_end (&prop_dict));
    }

  g_free (pspecs);
  return g_variant_dict_end (&vardict);
}

gboolean
animations_dbus_validate_property_from_variant (GObject     *object,
                                                const char  *name,
                                                GVariant    *variant,
                                                GError     **error)
{
  GParamSpec *pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (object), name);

  if (pspec == NULL)
    {
      g_set_error (error,
                   ANIMATIONS_DBUS_ERROR,
                   ANIMATIONS_DBUS_ERROR_INVALID_SETTING,
                   "Animation does not have a setting name '%s'",
                   name);
      return FALSE;
    }

  g_auto(GValue) value = G_VALUE_INIT;
  g_dbus_gvariant_to_gvalue (variant, &value);

  /* Return value is whether modifying the value
   * was necessary to ensure compliance with the
   * constraints, we want to return an error in
   * that case. */
  if (g_param_value_validate (pspec, &value))
    {
      g_set_error (error,
                   ANIMATIONS_DBUS_ERROR,
                   ANIMATIONS_DBUS_ERROR_INVALID_SETTING,
                   "Invalid value for setting '%s'",
                   name);
      return FALSE;
    }

  return TRUE;
}

gboolean
animations_dbus_set_property_from_variant (GObject     *object,
                                           const char  *name,
                                           GVariant    *variant,
                                           GError     **error)
{
  if (!animations_dbus_validate_property_from_variant (object,
                                                       name,
                                                       variant,
                                                       error))
    return FALSE;

  g_auto(GValue) value = G_VALUE_INIT;
  g_dbus_gvariant_to_gvalue (variant, &value);

  g_object_set_property (object, name, &value);
  return TRUE;
}

/* XXX: Borrowing this struct from GDBus-Codegen is not ideal,
 *      but it does not appear that there is a method
 *      to get a D-Bus property name for a hyphenated name. */
typedef struct
{
  GDBusPropertyInfo parent_struct;
  const gchar *hyphen_name;
  gboolean use_gvariant;
} ExtendedGDBusPropertyInfo;

static const char *
lookup_dbus_prop_name_on_interface (GDBusInterfaceInfo *info,
                                    const char         *name)
{
  GDBusPropertyInfo **properties = info->properties;

  for (GDBusPropertyInfo **property_info_iter = properties;
       *property_info_iter != NULL;
       ++property_info_iter)
    {
      ExtendedGDBusPropertyInfo *extended_info = (ExtendedGDBusPropertyInfo *) *property_info_iter;

      if (g_strcmp0 (name, extended_info->hyphen_name) == 0)
        return (*property_info_iter)->name;
    }

  g_assert_not_reached ();
  return NULL;
}

void
animations_dbus_emit_properties_changed_for_skeleton_properties (GDBusInterfaceSkeleton *skeleton,
                                                                 const char * const     *props)
{
  g_auto(GVariantBuilder) changed_builder;
  g_auto(GVariantBuilder) invalidated_builder;

  g_variant_builder_init (&changed_builder, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_init (&invalidated_builder, G_VARIANT_TYPE("as"));

  /* No work to do, return early */
  if (*props == NULL)
    return;

  GObjectClass *object_class = G_OBJECT_GET_CLASS (G_OBJECT (skeleton));
  GDBusInterfaceInfo *interface_info = g_dbus_interface_skeleton_get_info (skeleton);

  for (const char * const *props_iter = props; *props_iter != NULL; ++props_iter)
    {
      GParamSpec *pspec = g_object_class_find_property (object_class, *props_iter);

      g_assert (pspec != NULL);
      g_auto(GValue) value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);
      g_object_get_property (G_OBJECT (skeleton), *props_iter, &value);

      /* Non-floating reference */
      g_autoptr(GVariant) variant =
        g_dbus_gvalue_to_gvariant (&value, value_type_to_variant_type (pspec->value_type));
      g_variant_builder_add (&changed_builder,
                             "{sv}",
                             lookup_dbus_prop_name_on_interface (interface_info,
                                                                 *props_iter),
                             variant);
    }

  g_autoptr(GVariant) properties_changed_variant =
    g_variant_ref_sink (g_variant_new ("(sa{sv}as)",
                                       interface_info->name,
                                       &changed_builder,
                                       &invalidated_builder));
  g_autoptr(GList) connections = g_dbus_interface_skeleton_get_connections (skeleton);

  for (GList *l = connections; l != NULL; l = l->next)
    {
      GDBusConnection *connection = l->data;

      g_dbus_connection_emit_signal (connection,
                                     NULL,
                                     g_dbus_interface_skeleton_get_object_path (skeleton),
                                     "org.freedesktop.DBus.Properties",
                                     "PropertiesChanged",
                                     properties_changed_variant,
                                     NULL);
    }
}

