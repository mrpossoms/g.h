#include ".test.h"
#include "g.h"
#include "g.game.object.h"

struct my_core : public g::core
{
    virtual bool initialize()
    {
        return true;
    }

    virtual void update(float dt)
    {
    }
};

/**
 * A test is nothing more than a stripped down C program
 * returning 0 is success. Use asserts to check for errors
 */
TEST
{
    g::asset::store assets;
    my_core core;

    core.start({});

    // g::game::object foo(&assets, "data/foo.yaml", {
    //     { "traits", {
    //             { "hp", 99 },
    //             { "name", "foo" },
    //             { "speed", 1} }
    //     }
    // });

    // assert(strncmp(foo.traits()["name"].string, "foo", 3) == 0);
    // assert(foo.traits()["hp"].number == 99);
    // assert(foo.traits()["speed"].number == 1);

    // unlink("data/foobar.yaml");

    // // save a default config object for something that doesn't exist
    // g::game::object foobar(&assets, "data/foobar.yaml", {
    //     { "traits", {
    //             { "hp", 100 },
    //             { "name", "foobar" }}
    //     }
    // });    
    // assert(foobar.traits()["hp"].number == 100);
    // assert(strcmp(foobar.traits()["name"].string, "foobar") == 0);

    // // ensure non-default values are loaded
    // g::game::object foobar1(&assets, "data/foobar.yaml", {
    //     { "traits", {
    //             { "hp", 99 },
    //             { "name", "foodar" }}
    //     }
    // });
    // assert(foobar1.traits()["hp"].number == 100);
    // assert(strcmp(foobar1.traits()["name"].string, "foobar") == 0);

    // make sure a texture resources can be created
    g::game::object car(&assets, "data/car.yaml", {
        { "traits", {
                { "hp", 204 },
                { "grip", 1 },
                { "name", "zoomer" }}},
        { "textures", {
                { "skin", "zoomer.skin.png" }}},
        { "geometry", {
                { "body", "body.obj"}}},
    });
    // assert(g::io::file{"data/tex/zoomer.skin.png"}.exists());
    // assert(g::io::file{"data/geo/body.obj"}.exists());

	return 0;
}
