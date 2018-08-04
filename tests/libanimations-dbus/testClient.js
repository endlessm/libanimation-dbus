const {
    AnimationsDbus,
    Gio,
    GLib,
    GObject
} = imports.gi;

const Lang = imports.lang;
const System = imports.system;

const FakeServerSurfaceBridge = new Lang.Class({
    Name: 'FakeServerSurfaceBridge',
    Extends: GObject.Object,
    Implements: [ AnimationsDbus.ServerSurfaceBridge ],
    Properties: {
        title: GObject.ParamSpec.string('title',
                                        'Title',
                                        'Surface Title',
                                        GObject.ParamFlags.READWRITE |
                                        GObject.ParamFlags.CONSTRUCT,
                                        'Default Title'),
        geometry: GObject.param_spec_variant('geometry',
                                             'Geometry',
                                             'Surface Geometry',
                                             new GLib.VariantType('(iiii)'),
                                             new GLib.Variant('(iiii)', [0, 0, 1, 1]),
                                             GObject.ParamFlags.READWRITE |
                                             GObject.ParamFlags.CONSTRUCT)
    },

    _init: function(props) {
        this.parent(props);

        this._effects = {};
    },

    vfunc_attach_effect: function(event, effect) {
        if (event !== 'move')
            throw new GLib.Error(AnimationsDbus.error_quark(),
                                 AnimationsDbus.Error.UNSUPPORTED_EVENT_FOR_ANIMATION_EFFECT,
                                 `Unsupported event ${event}`);

        return new FakeAttachedAnimationEffect({ bridge: effect.bridge });
    },

    vfunc_detach_effect: function(event, effect) {
    },

    vfunc_get_title: function() {
        return this.title;
    },

    vfunc_get_geometry: function() {
        return this.geometry;
    },

    vfunc_get_available_effects: function() {
        return {
            'move': ['fake-effect']
        };
    }
});

const FakeAnimationEffectBridge = new Lang.Class({
    Name: 'FakeAnimationEffectBridge',
    Extends: GObject.Object,
    Implements: [ AnimationsDbus.ServerEffectBridge ],
    Properties: {
        some_property: GObject.ParamSpec.int('some-property',
                                             'Some Property',
                                             'Some long property description',
                                             GObject.ParamFlags.READWRITE |
                                             GObject.ParamFlags.CONSTRUCT,
                                             0,
                                             10,
                                             5)
    },

    vfunc_get_name: function() {
        return 'fake-effect';
    }
});

const FakeAttachedAnimationEffect = new Lang.Class({
    Name: 'FakeAttachedAnimationEffect',
    Extends: GObject.Object,
    Implements: [ AnimationsDbus.ServerSurfaceAttachedEffect ],
    Properties: {
        bridge: GObject.ParamSpec.object('bridge',
                                         'FakeAnimationEffectBridge',
                                         'The FakeAnimationEffectBridge that is the metaclass',
                                         GObject.ParamFlags.READWRITE |
                                         GObject.ParamFlags.CONSTRUCT_ONLY,
                                         FakeAnimationEffectBridge)
    }
});

const FakeAnimationEffectBridgeProvider = new Lang.Class({
    Name: 'FakeAnimationEffectBridgeProvider',
    Extends: GObject.Object,
    Implements: [ AnimationsDbus.ServerEffectFactory ],

    vfunc_create_effect: function(name, settings) {
        switch (name) {
        case 'fake-effect':
            return new FakeAnimationEffectBridge({});
            break;
        default:
            throw new GLib.Error(AnimationsDbus.error_quark(),
                                 AnimationsDbus.Error.NO_SUCH_ANIMATION,
                                 `Cannot create animaton with name ${name}`);
        }   
    }
});

function reportGError(error) {
    expect(`Error ${error.domain}: ${error.code} "${error.message}" occurred`).toEqual('');
}

function doneHandlerExceptionOnly(done, func) {
    return (...args) => {
        try {
            func(...args);
        } catch (e) {
            reportGError(e);
            done();
        }
    }
}

function doneHandler(done, func) {
    return (...args) => {
        try {
            func(...args);
            done();
        } catch (e) {
            reportGError(e);
            done();
        }
    }
}

// Taken from libdmodel
const customMatchers = {
    toBeA: function (util, customEqualityTesters) {
        return {
            compare: function (object, expectedType) {
                let result = {
                    pass: function () {
                        return object instanceof expectedType;
                    }()
                }

                let objectTypeName;
                if (typeof object === 'object')
                    objectTypeName = object.constructor.name;
                else
                    objectTypeName = typeof widget;

                let expectedTypeName;
                if (typeof expectedType.$gtype !== 'undefined')
                    expectedTypeName = expectedType.$gtype.name;
                else
                    expectedTypeName = expectedType;

                if (result.pass) {
                    result.message = 'Expected ' + object + ' not to be a ' + expectedTypeName + ', but it was';
                } else {
                    result.message = 'Expected ' + object + ' to be a ' + expectedTypeName + ', but instead it had type ' + objectTypeName;
                }
                return result;
            }
        }
    }
};

describe('Animations DBus', function() {
    let testDBus = Gio.TestDBus.new(Gio.TestDBusFlags.NONE);
    let serverConnection = null;
    let clientConnection = null;

    beforeAll(function() {
        jasmine.addMatchers(customMatchers);
    });

    beforeEach(function() {
        testDBus.up();

        serverConnection =
            Gio.DBusConnection.new_for_address_sync(testDBus.get_bus_address(),
                                                    Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT |
                                                    Gio.DBusConnectionFlags.MESSAGE_BUS_CONNECTION,
                                                    null,
                                                    null);
        clientConnection =
            Gio.DBusConnection.new_for_address_sync(testDBus.get_bus_address(),
                                                    Gio.DBusConnectionFlags.AUTHENTICATION_CLIENT |
                                                    Gio.DBusConnectionFlags.MESSAGE_BUS_CONNECTION,
                                                    null,
                                                    null);
    });

    afterEach(function() {
        testDBus.down();
    });

    describe('Server', function() {
        let server = null;

        beforeEach(function(done) {
            let provider = new FakeAnimationEffectBridgeProvider({});
            AnimationsDbus.Server.new_with_connection_async(provider,
                                                            serverConnection,
                                                            null,
                                                            doneHandler(done, function(source, result) {
                server = AnimationsDbus.Server.new_finish(source, result);
            }));
        });

        afterEach(function() {
            server = null;
        });

        describe('with a connected Client', function() {
            let client = null;

            beforeEach(function(done) {
                AnimationsDbus.Client.new_with_connection_async(clientConnection,
                                                                null,
                                                                doneHandler(done, function(source, result) {
                    client = AnimationsDbus.Client.new_finish(source, result);
                }));
            });

            it('can create a known animation effect', function(done) {
                client.create_animation_effect_async('My cool effect',
                                                     'fake-effect',
                                                     new GLib.Variant('a{sv}', {}),
                                                     null,
                                                     doneHandler(done, function(source, result) {
                    let effect = source.create_animation_effect_finish(result);
                    expect(effect.title).toBe('My cool effect');
                }));
            });

            it('can create a known animation effect with custom settings', function(done) {
                client.create_animation_effect_async('My cool effect',
                                                     'fake-effect',
                                                     new GLib.Variant('a{sv}', {
                                                         'some-property': new GLib.Variant('i', 2)
                                                     }),
                                                     null,
                                                     doneHandler(done, function(source, result) {
                    let effect = source.create_animation_effect_finish(result);
                    expect(effect.settings.deep_unpack()['some-property'].deep_unpack()).toBe(
                        2
                    );
                }));
            });

            // Skipped for now, since we don't have any way of validating
            // settings on construction.
            xit('creating a known animation effect with invalid setting name throws', function(done) {
                client.create_animation_effect_async('My cool effect',
                                                     'fake-effect',
                                                     new GLib.Variant('a{sv}', {
                                                         'bad-property': new GLib.Variant('i', 2)
                                                     }),
                                                     null,
                                                     doneHandler(done, function(source, result) {
                    expect(function() {
                        source.create_animation_effect_finish(result);
                    }).toThrow();
                }));
            });

            // Skipped for now, since we don't have any way of validating
            // settings on construction.
            xit('creating a known animation effect with invalid setting value throws', function(done) {
                client.create_animation_effect_async('My cool effect',
                                                     'fake-effect',
                                                     new GLib.Variant('a{sv}', {
                                                         'some-property': new GLib.Variant('i', 100)
                                                     }),
                                                     null,
                                                     doneHandler(done, function(source, result) {
                    expect(function() {
                        source.create_animation_effect_finish(result);
                    }).toThrow();
                }));
            });

            // Skipped, since throwing errors across vfunc boundaries
            // in Gjs does not appear to work correctly.
            xit('creating an unknown animation effect throws', function(done) {
                client.create_animation_effect_async('My cool effect',
                                                     'unknown-effect',
                                                     new GLib.Variant('a{sv}', {}),
                                                     null,
                                                     doneHandler(done, function(source, result) {
                    expect(function() {
                        source.create_animation_effect_finish(result);
                    }).toThrow();
                }));
            });

            describe('with an attached surface', function() {
                let surface = null;
                let serverSurface = null;

                beforeEach(function() {
                    surface = new FakeServerSurfaceBridge({});
                    serverSurface = server.register_surface(surface);
                });

                it('shows up in the client surface listing', function(done) {
                    client.list_surfaces_async(null, doneHandler(done, function(source, result) {
                        let surfaces = source.list_surfaces_finish(result);

                        expect(surfaces.length).toBe(1);
                    }));
                });

                it('changed geometry is reflected in properties if changed before querying', function(done) {
                    surface.geometry = new GLib.Variant('(iiii)', [100, 100, 200, 200]);
                    client.list_surfaces_async(null, doneHandler(done, function(source, result) {
                        let surfaces = source.list_surfaces_finish(result);

                        expect(surfaces[0].geometry.deep_unpack()).toEqual([100, 100, 200, 200]);
                    }));
                });

                describe('first surface in client surface listing', function() {
                    let clientSurface = null;

                    beforeEach(function(done) {
                        client.list_surfaces_async(null, doneHandler(done, function(source, result) {
                            let surfaces = source.list_surfaces_finish(result);

                            clientSurface = surfaces[0];
                        }));
                    });

                    it('has the expected object path', function() {
                         expect(clientSurface.proxy.get_object_path()).toBe(
                             '/com/endlessm/Libanimation/AnimatableSurface/0'
                         );
                     });

                     it('has the expected title', function() {
                         expect(clientSurface.title).toBe('Default Title');
                     });

                     it('changed title is not reflected in properties if changed after querying', function() {
                         surface.title = 'New Title';
                         expect(clientSurface.title).not.toEqual('newTitle');
                     });

                     it('changed title is reflected in properties after emitting the change signal', function(done) {
                         let conn = clientSurface.proxy.connect('notify::title', doneHandler(done, function() {
                             expect(clientSurface.title).toEqual('New Title');
                             clientSurface.proxy.disconnect(conn);
                         }));
                         surface.title = 'New Title';
                         serverSurface.emit_title_changed();
                     });

                     it('has the expected geometry', function() {
                         expect(clientSurface.geometry.deep_unpack()).toEqual([0, 0, 1, 1]);
                     });

                     it('changed geometry is not reflected in properties if changed after querying', function() {
                         surface.geometry = new GLib.Variant('(iiii)', [100, 100, 200, 200]);
                         expect(clientSurface.geometry.deep_unpack()).not.toEqual([100, 100, 200, 200]);
                     });

                     it('changed geometry is reflected in properties after emitting the change signal', function(done) {
                         let conn = clientSurface.proxy.connect('notify::geometry', doneHandler(done, function() {
                             expect(clientSurface.geometry.deep_unpack()).toEqual([100, 100, 200, 200]);
                             clientSurface.proxy.disconnect(conn);
                         }));
                         surface.geometry = new GLib.Variant('(iiii)', [100, 100, 200, 200]);
                         serverSurface.emit_geometry_changed();
                     });
                });
            });

            describe('with a created animation effect', function() {
                let effect = null;

                beforeEach(function(done) {
                    client.create_animation_effect_async('My cool effect',
                                                         'fake-effect',
                                                         new GLib.Variant('a{sv}', {}),
                                                         null,
                                                         doneHandler(done, function(source, result) {
                        effect = source.create_animation_effect_finish(result);
                    }));
                });

                it('has the expected object path', function() {
                    expect(effect.proxy.get_object_path()).toBe(
                        '/com/endlessm/Libanimation/AnimationManager/1/AnimationEffect/0'
                    );
                });

                it('has a property in its schema called some-property', function() {
                    expect(Object.keys(effect.schema.deep_unpack())).toContain(
                        'some-property'
                    );
                });

                it('the type of some-property is i', function() {
                    expect(effect.schema.deep_unpack()['some-property'].deep_unpack().type.deep_unpack()).toBe(
                        'i'
                    );
                });

                it('the default of some-property is 5', function() {
                    expect(effect.schema.deep_unpack()['some-property'].deep_unpack().default.deep_unpack()).toBe(
                        5
                    );
                });

                it('the min of some-property is 0', function() {
                    expect(effect.schema.deep_unpack()['some-property'].deep_unpack().min.deep_unpack()).toBe(
                        0
                    );
                });

                it('the max of some-property is 10', function() {
                    expect(effect.schema.deep_unpack()['some-property'].deep_unpack().max.deep_unpack()).toBe(
                        10
                    );
                });

                it('has the a key in its settings called some-property', function() {
                    expect(Object.keys(effect.settings.deep_unpack())).toContain(
                        'some-property'
                    );
                });

                it('the value of some-property is 5', function() {
                    expect(effect.settings.deep_unpack()['some-property'].deep_unpack()).toBe(
                        5
                    );
                });

                it('can change settings on that effect', function(done) {
                    effect.change_setting_async('some-property',
                                                new GLib.Variant('i', 2),
                                                null,
                                                doneHandler(done, function(source, result) {
                        expect(source.change_setting_finish(result)).toBeTruthy();
                    }));
                });

                it('changed settings on that effect get reflected in the properties', function(done) {
                    let conn = effect.proxy.connect('notify::settings', function() {
                        expect(effect.settings.deep_unpack()['some-property'].deep_unpack()).toBe(2);
                        effect.proxy.disconnect(conn);
                        done();
                    })
                    effect.change_setting_async('some-property',
                                                new GLib.Variant('i', 2),
                                                null,
                                                doneHandler(done, function(source, result) {
                        expect(source.change_setting_finish(result)).toBeTruthy();
                    }));
                });

                it('changing setting to invalid value throws', function(done) {
                    effect.change_setting_async('some-property',
                                                new GLib.Variant('i', 100),
                                                null,
                                                doneHandler(done, function(source, result) {
                        expect(function() {
                            source.change_setting_finish(result);
                        }).toThrow();
                    }));
                });

                describe('with some attached surfaces', function() {
                    let serverSurface1 = null;
                    let serverSurface2 = null;
                    let clientSurfaces = null;

                    beforeEach(function(done) {
                        serverSurface1 = server.register_surface(new FakeServerSurfaceBridge({
                            title: 'Server Surface 1'
                        }));
                        serverSurface2 = server.register_surface(new FakeServerSurfaceBridge({
                            title: 'Server Surface 2'
                        }));
                        clientSurfaces = null;

                        client.list_surfaces_async(null, doneHandler(done, function(source, result) {
                            clientSurfaces = source.list_surfaces_finish(result);
                        }));
                    });

                    it('can be attached to the move event', function(done) {
                        clientSurfaces[0].attach_effect_async('move', effect, null, doneHandler(done, function(source, result) {
                            expect(source.attach_effect_finish(result)).toBe(true);
                        }));
                    });

                    xit('throws if attached to a unsupported event', function(done) {
                        clientSurfaces[0].attach_effect_async('unsupported', effect, null, doneHandler(done, function(source, result) {
                            expect(function() {
                                source.attach_effect_finish(result)
                            }).toThrow();
                        }));
                    });

                    it('has move event in the effects property when attached', function(done) {
                        let conn = clientSurfaces[0].proxy.connect('notify::effects', doneHandler(done, function() {
                            expect(Object.keys(clientSurfaces[0].effects.deep_unpack())).toContain('move');
                            clientSurfaces[0].proxy.disconnect(conn);
                        }));
                        clientSurfaces[0].attach_effect_async('move', effect, null, doneHandlerExceptionOnly(done, function(source, result) {
                            expect(source.attach_effect_finish(result)).toBe(true);
                        }));
                    });

                    it('effect for move event when attached is of expected object path', function(done) {
                        let conn = clientSurfaces[0].proxy.connect('notify::effects', doneHandler(done, function() {
                            expect(clientSurfaces[0].effects.deep_unpack()['move'].deep_unpack()).toContain(
                                effect.get_object_path()
                            );
                            clientSurfaces[0].proxy.disconnect(conn);
                        }));
                        clientSurfaces[0].attach_effect_async('move', effect, null, doneHandlerExceptionOnly(done, function(source, result) {
                            expect(source.attach_effect_finish(result)).toBe(true);
                        }));
                    });

                    describe('with attached effect for move event', function() {
                        beforeEach(function(done) {
                            clientSurfaces[0].attach_effect_async('move', effect, null, doneHandler(done, function(source, result) {
                                source.attach_effect_finish(result);
                            }));
                        });

                        it('is the highest priority event on the server side', function() {
                            let attachedEffect = serverSurface1.highest_priority_attached_effect_for_event('move');

                            expect(attachedEffect).toBeA(FakeAttachedAnimationEffect);
                        });

                        it('is removed when the server connection closes', function(done) {
                            server.connect('client-disconnected', doneHandler(done, function() {
                                let attachedEffect = serverSurface1.highest_priority_attached_effect_for_event('move');
                                expect(attachedEffect).toBe(null);
                            }));

                            // Close the connection. This will cause the name to be lost
                            // and state to be cleaned up on the server side.
                            clientConnection.close(null, doneHandlerExceptionOnly(done, function(source, result) {
                                clientConnection.close_finish(result);
                            }));
                        });

                        describe('with an attached server-priority effect for move event', function() {
                            let serverAnimationManager = null;
                            let serverEffect = null;

                            beforeEach(function() {
                                serverAnimationManager = server.create_animation_manager();
                                serverEffect = serverAnimationManager.create_effect('Server Effect',
                                                                                    'fake-effect',
                                                                                    new GLib.Variant('a{sv}',
                                                                                                     { some_property: new GLib.Variant('i', 7) }));

                                serverSurface1.attach_animation_effect_with_server_priority('move', serverEffect);
                            });

                            // Need to explicitly destroy the server effect to avoid
                            // calling back into the JSAPI from dispose()
                            afterEach(function() {
                                if (serverEffect !== null) {
                                    serverEffect.destroy();
                                    serverEffect = null;
                                }
                            });

                            it('takes priority over server attached effects', function() {
                                let attachedEffect = serverSurface1.highest_priority_attached_effect_for_event('move');

                                expect(attachedEffect.bridge.some_property).not.toBe(7);
                            });

                            it('is overtaken by server attached effects when deleted', function(done) {
                                clientSurfaces[0].detach_effect_async('move', effect, null, doneHandler(done, function(source, result) {
                                    source.detach_effect_finish(result);

                                    let attachedEffect = serverSurface1.highest_priority_attached_effect_for_event('move');
                                    expect(attachedEffect.bridge.some_property).toBe(7);
                                }));
                            });

                            it('is overtaken by server attached effects when the client disconnects', function(done) {
                                server.connect('client-disconnected', doneHandler(done, function() {
                                    let attachedEffect = serverSurface1.highest_priority_attached_effect_for_event('move');
                                    expect(attachedEffect.bridge.some_property).toBe(7);
                                }));

                                // Close the connection. This will cause the name to be lost
                                // and state to be cleaned up on the server side.
                                clientConnection.close(null, doneHandlerExceptionOnly(done, function(source, result) {
                                    clientConnection.close_finish(result);
                                }));
                            });
                        });
                    });
                });
            });
        });
    });
});
