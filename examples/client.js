// /examples/client.js
//
// Copyright (c) 2018 Endless Mobile Inc.
//
// Example client for AnimationsDbus

const AnimationsDbus = imports.gi.AnimationsDbus;
const GLib = imports.gi.GLib;

const Mainloop = imports.mainloop;

function boot() {
  AnimationsDbus.Client.new_async(null, (source, result) => {
    let client = AnimationsDbus.Client.new_finish(source, result);

    log('Got client, listing surfaces');
    client.list_surfaces_async(null, (source, result) => {
      let surfaces = source.list_surfaces_finish(result);

      if (surfaces.length > 0) {
        surfaces[0].list_available_effects_async(null, (source, result) => {
          let effects = source.list_available_effects_finish(result);
          let event = Object.keys(effects)[0];
          let selectedEffectName = effects[event][0];

          log(`Available effects on "${surfaces[0].title}" are: ${JSON.stringify(effects, null, 2)}`);
          log(`Creating effect ${selectedEffectName} on the bus`);

          client.create_animation_effect_async(`My special ${selectedEffectName} effect`,
                                               selectedEffectName,
                                               new GLib.Variant('a{sv}', {}),
                                               null,
                                               (source, result) => {
            let effect = source.create_animation_effect_finish(result);
            log(`Created effect with object path: ${effect.get_object_path()}`);
            log(`Effect settings are: ${JSON.stringify(effect.settings.deep_unpack())}`);
            log(`Effect schema is: ${JSON.stringify(effect.schema.deep_unpack())}`);

            log(`Attaching effect to surface: "${surfaces[0].title}"`);
            surfaces[0].attach_effect_async(event, effect, null, (source, result) => {
              surfaces[0].attach_effect_finish(result);
              log(`Attached effect ${selectedEffectName} "${surfaces[0].title}"`);
            });
          });
        });
      }
    });
  });
}

GLib.idle_add(GLib.PRIORITY_DEFAULT_IDLE, boot);
Mainloop.run();


