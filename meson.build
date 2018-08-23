# Copyright 2018 Endless Mobile, Inc.

project('animations-dbus', 'c', version: '0.0.0', license: 'LGPL2.1+',
    meson_version: '>= 0.47.1',
    default_options: ['c_std=c99', 'warning_level=3'])

gnome = import('gnome')
pkg = import('pkgconfig')

version_components = meson.project_version().split('.')
api_version = version_components[0]
api_name = '@0@-@1@'.format(meson.project_name(), api_version)
libtool_version = '0.0.0'
namespace_name = 'AnimationsDbus'

gio = dependency('gio-2.0')
gio_unix = dependency('gio-unix-2.0')
glib = dependency('glib-2.0')
gobject = dependency('gobject-2.0')

config = configuration_data()
config.set_quoted('ANIMATIONS_DBUS_VERSION', meson.project_version())
configure_file(configuration: config, output: 'config.h')

jasmine_report_argument = ''
junit_reports_dir = get_option('jasmine_junit_reports_dir')
if junit_reports_dir != ''
    jasmine_report_argument = '--junit=@0@/${log/.log/.js.xml}'.format(junit_reports_dir)
endif

subdir('data')
subdir('animations-dbus')
subdir('tests')

requires = [gio, gio_unix, glib, gobject]

pkg.generate(filebase: api_name, libraries: [main_library],
    description: 'DBus API for remotely programmable animations',
    name: meson.project_name(), subdirs: api_name, requires: requires,
    url: 'http://endlessm.github.io/libanimations-dbus',
    version: meson.project_version())

