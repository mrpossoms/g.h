#include ".test.h"
#include "g.h"

/**
 * A test is nothing more than a stripped down C program
 * returning 0 is success. Use asserts to check for errors
 */
TEST
{
    using namespace g::dyn;

    { // plane facing up in the +y direction
        // ray over the origin
        auto inter = cd::ray_plane(cd::ray{{ 0, 1, 0}, { 0, -1, 0}}, {0, 0, 0}, {0, 1, 0});
        assert(near(inter.time, 1));

        // ray positioned randomly above the plane
        auto y = (random() % 100) + 1.f;
        inter = cd::ray_plane(cd::ray{{(random() % 100) - 50.f, y, (random() % 100) - 50.f}, { 0, -1, 0}}, {0, 0, 0}, {0, 1, 0});
        assert(near(inter.time, y));

        // ray over the origin
        // <-1   2->
        inter = cd::ray_plane(cd::ray{{ 0, 1, 0}, { 0, -1, 0}}, {0, 2, 0}, {0, 1, 0});
        assert(std::isnan(inter.time));
    }

    { // plane facing up in the +x direction
        // ray over the origin
        auto inter = cd::ray_plane(cd::ray{{ 1, 0, 0}, { -1, 0, 0}}, {0, 0, 0}, {1, 0, 0});
        assert(near(inter.time, 1));

        // ray pointing away from the plane (no collision)
        inter = cd::ray_plane(cd::ray{{ 1, 0, 0}, { 1, 0, 0}}, {0, 0, 0}, {1, 0, 0});
        assert(std::isnan(inter.time));
    }

    { // plane facing up in the +z direction
        // ray over the origin
        auto inter = cd::ray_plane(cd::ray{{ 0, 0, 1}, { 0, 0, -1}}, {0, 0, 0}, {0, 0, 1});
        assert(near(inter.time, 1));

        // plane is above the ray (no collision)
        inter = cd::ray_plane(cd::ray{{ 0, 0, 1}, { 0, 0, -1}}, {0, 0, 2}, {0, 0, 1});
        assert(std::isnan(inter.time));
    }

	return 0;
}
