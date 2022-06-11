#include ".test.h"
#include "g.h"

using namespace xmath;

vec<3> world_to_screen(const mat<4,4>& transform, const vec<3>& p)
{
    std::cerr << __func__ << std::endl;
    auto p_proj = transform * vec<4>{ p[0], p[1], p[2], 1 };
    std::cerr << p_proj.to_string() << std::endl;
    return (p_proj / p_proj[3]).slice<3>();
}

vec<3> screen_to_world(const mat<4,4>& proj, const mat<4,4>& view, const vec<3>& p)
{
    std::cerr << __func__ << std::endl;
    float z = -1.f / ((p[2] * proj[3][2]) - proj[2][2]);
    float w = z * proj[3][2];
    auto p_screen = vec<4>{p[0] * w, p[1] * w, z, w};

    auto p_view = proj.invert() * p_screen;
    auto p_world = view.invert() * p_view;

    // "   float z = -1.0 / ((p[2] * P[3][2]) - P[2][2]);"
    // "   float w = z * P[3][2];"
    // "   vec4 p_w = vec4(p.xy * w, z, w);"
    return p_world.slice<3>();
}

/**
 * A test is nothing more than a stripped down C program
 * returning 0 is success. Use asserts to check for errors
 */
TEST
{
    { // ensure that a point can be retrieved after projection
        auto P = mat<4,4>::perspective(1, 100, M_PI / 2, 1.f);
        auto V = mat<4, 4>::I();

        auto p = vec<3>{0, 0, -10};
        auto p_prime = world_to_screen(P, p);

        auto p0 = screen_to_world(P, V, p_prime);

        assert(p0.is_near(p, 0.000001));
    }

    { // simple case with light behind viewer

    }


	return 0;
}
