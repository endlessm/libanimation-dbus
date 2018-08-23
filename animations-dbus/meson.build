# Copyright (c) 2018 Endless Mobile, Inc.

version = configuration_data()
version.set('ANIMATIONS_DBUS_MAJOR_VERSION', version_components[0])
version.set('ANIMATIONS_DBUS_MINOR_VERSION', version_components[1])
version.set('ANIMATIONS_DBUS_MICRO_VERSION', version_components[2])
version_h = configure_file(configuration: version,
    input: 'animations-dbus-version.h.in', output: 'animations-dbus-version.h')

gdbus_targets = gnome.gdbus_codegen('animations-dbus-objects',
    '../data/com.endlessm.Libanimation.xml',
    interface_prefix: 'com.endlessm.Libanimation',
    namespace: 'AnimationsDbus',
    install_header: true,
    install_dir: join_paths(get_option('includedir'), api_name, 'animations-dbus'))

installed_headers = [
    'animations-dbus-client-effect.h',
    'animations-dbus-client-object.h',
    'animations-dbus-client-surface.h',
    'animations-dbus-errors.h',
    'animations-dbus-server.h',
    'animations-dbus-server-animation-manager.h',
    'animations-dbus-server-effect.h',
    'animations-dbus-server-effect-bridge-interface.h',
    'animations-dbus-server-effect-factory-interface.h',
    'animations-dbus-server-object.h',
    'animations-dbus-server-surface.h',
    'animations-dbus-server-surface-attached-effect-interface.h',
    'animations-dbus-server-surface-bridge-interface.h',
    'animations-dbus-server-types.h',
    version_h
]
private_headers = [
    'animations-dbus-main-context-private.h',
    'animations-dbus-server-skeleton-properties.h'
]
sources = [
    gdbus_targets[0],
    gdbus_targets[1],
    'animations-dbus-client-effect.c',
    'animations-dbus-client-object.c',
    'animations-dbus-client-surface.c',
    'animations-dbus-main-context-private.c',
    'animations-dbus-errors.c',
    'animations-dbus-server-animation-manager.c',
    'animations-dbus-server-effect.c',
    'animations-dbus-server-effect-bridge-interface.c',
    'animations-dbus-server-effect-factory-interface.c',
    'animations-dbus-server-object.c',
    'animations-dbus-server-skeleton-properties.c',
    'animations-dbus-server-surface.c',
    'animations-dbus-server-surface-attached-effect-interface.c',
    'animations-dbus-server-surface-bridge-interface.c'
]

include = include_directories('.')

main_library = shared_library('@0@-@1@'.format(meson.project_name(), api_version),
    sources, installed_headers, private_headers,
    c_args: ['-DG_LOG_DOMAIN="@0@"'.format(namespace_name),
        '-DCOMPILING_ANIMATIONS_DBUS'],
    dependencies: [gio, gio_unix, glib, gobject],
    include_directories: include, install: true,
    soversion: api_version, version: libtool_version)

introspection_sources = [
    sources,
    join_paths(meson.build_root(), 'animations-dbus', 'animations-dbus-objects.h'),
    installed_headers
]

gnome.generate_gir(main_library, extra_args: ['--warn-all', '--warn-error'],
    identifier_prefix: 'AnimationsDbus', include_directories: include,
    includes: ['Gio-2.0', 'GLib-2.0', 'GObject-2.0'],
    install: true, namespace: namespace_name, nsversion: api_version,
    sources: introspection_sources, symbol_prefix: 'animations_dbus')

install_headers(installed_headers, subdir: api_name)
