#include ".test.h"
#include "g.game.object.h"

/**
 * A test is nothing more than a stripped down C program
 * returning 0 is success. Use asserts to check for errors
 */
TEST
{
    g::asset::store assets;

    g::game::object foo(&assets, "data/foo.yaml", {
        { "hp", 100 }
    });

    assert(strncmp(foo.traits()["name"].string, "foo", 3) == 0);
    assert(foo.traits()["hp"].number == 99);
    assert(foo.traits()["speed"].number == 1);

    unlink("data/foobar.yaml");

    // save a default config object for something that doesn't exist
    g::game::object foobar(&assets, "data/foobar.yaml", {
        { "hp", 100 },
        { "name", "foobar" }
    });    
    assert(foobar.traits()["hp"].number == 100);
    assert(strcmp(foobar.traits()["name"].string, "foobar") == 0);

    // ensure non-default values are loaded
    g::game::object foobar1(&assets, "data/foobar.yaml", {
        { "hp", 99 },
        { "name", "foodar" }
    });
    assert(foobar1.traits()["hp"].number == 100);
    assert(strcmp(foobar1.traits()["name"].string, "foobar") == 0);


	return 0;
}
