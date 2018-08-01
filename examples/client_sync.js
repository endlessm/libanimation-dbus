// /examples/client.js
//
// Copyright (c) 2018 Endless Mobile Inc.
//
// Example client for AnimationsDbus

const AnimationsDbus = imports.gi.AnimationsDbus;
const GLib = imports.gi.GLib;

const Console = imports.console;

let client = AnimationsDbus.Client.new();

log('Got client, listing surfaces');
let surfaces = client.list_surfaces();

if (surfaces.length > 0) {
  let effects = surfaces[0].list_available_effects();
  let event = Object.keys(effects)[0];
  let selectedEffectName = effects[event][0];

  log(`Available effects on "${surfaces[0].title}" are: ${JSON.stringify(effects, null, 2)}`);
  log(`Creating effect ${selectedEffectName} on the bus`);

  let effect = client.create_animation_effect(`My special ${selectedEffectName} effect`,
                                              selectedEffectName,
                                              new GLib.Variant('a{sv}', {}));

  log(`Created effect with object path: ${effect.get_object_path()}`);
  log(`Effect settings are: ${JSON.stringify(effect.settings.deep_unpack())}`);
  log(`Effect schema is: ${JSON.stringify(effect.schema.deep_unpack())}`);
  log(`Attaching effect to surface: "${surfaces[0].title}"`);

  surfaces[0].attach_effect(event, effect);
  log(`Attached effect ${selectedEffectName} "${surfaces[0].title}"`);
}

Console.interact();

