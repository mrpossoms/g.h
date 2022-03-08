#include ".test.h"
#include "g.game.object.h"

/**
 * A test is nothing more than a stripped down C program
 * returning 0 is success. Use asserts to check for errors
 */
TEST
{
    g::game::object foo("data/foo.yaml", {
        { "hp", 100 }
    });


	return 0;
}
