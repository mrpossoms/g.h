#include ".test.h"
#include "g.h"
#include "g.game.object.h"

struct my_core : public g::core
{
    float t = 0;

    virtual bool initialize()
    {
        return true;
    }

    virtual void update(float dt)
    {
    	if ((t += dt) > 1)
    	{
            running = false;
    	}
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

    core.start({
        .gfx = {
            .display = false,
        },
        .snd = {
            .enabled = false,
        }
    });

    g::game::object foo(&assets, "data/foo.yaml", {
        { "traits", {
                { "hp", 99.f },
                { "name", "foo" },
                { "speed", 1.f} }
        }
    });

    assert(strncmp(std::get<std::string>(foo.traits()["name"]).c_str(), "foo", 3) == 0);
    assert(std::get<float>(foo.traits()["hp"]) == 99.f);
    assert(std::get<float>(foo.traits()["speed"]) == 1.f);

    unlink("data/foobar.yaml");

    // save a default config object for something that doesn't exist
    g::game::object foobar(&assets, "data/foobar.yaml", {
        { "traits", {
                { "hp", 100.f },
                { "name", "foobar" }}
        }
    });    
    assert(std::get<float>(foobar.traits()["hp"]) == 100.f);
    assert(strcmp(std::get<std::string>(foobar.traits()["name"]).c_str(), "foobar") == 0);

    // ensure non-default values are loaded
    g::game::object foobar1(&assets, "data/foobar.yaml", {
        { "traits", {
                { "hp", 99.f },
                { "name", "foodar" }}
        }
    });
    assert(std::get<float>(foobar1.traits()["hp"]) == 100.f);
    assert(strcmp(std::get<std::string>(foobar1.traits()["name"]).c_str(), "foobar") == 0);

    // make sure a texture resources can be created
    g::game::object car(&assets, "data/car.yaml", {
        { "traits", {
                { "hp", 204.f },
                { "grip", 1.f },
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
