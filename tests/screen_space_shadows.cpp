#include ".test.h"
#include "g.h"

using namespace xmath;

vec<3> world_to_screen(const mat<4,4>& proj, const mat<4, 4>& view, const vec<3>& p)
{
    std::cerr << __func__ << std::endl;
    auto p_w = vec<4>{ p[0], p[1], p[2], 1 };
    std::cerr << p_w.to_string() << std::endl;
    auto p_view = view.transpose() * vec<4>{ p[0], p[1], p[2], 1 };
    std::cerr << "view " << p_view.to_string() << std::endl; 
    auto p_proj = proj * p_view;
    std::cerr << "screen " << p_proj.to_string() << std::endl;
    auto persp_divide = (p_proj / p_proj[3]).slice<3>();
    std::cerr << "persp div " << persp_divide.to_string() << std::endl;
    return persp_divide;
}

vec<3> screen_to_world(const mat<4,4>& proj, const mat<4,4>& view, const vec<3>& p)
{
    std::cerr << __func__ << std::endl;
    std::cerr << "persp div " << p.to_string() << std::endl;

    float z = -1.f / ((p[2] * proj[3][2]) - proj[2][2]);
    float w = z * proj[3][2];
    auto p_screen = vec<4>{p[0] * w, p[1] * w, z, w};
    std::cerr << "screen " << p_screen.to_string() << std::endl;

    auto p_view = proj.invert() * p_screen;
    std::cerr << "view " << p_view.to_string() << std::endl;
    auto p_world = view.transpose().invert() * p_view;
    std::cerr << "world " << p_view.to_string() << std::endl;

    // "   float z = -1.0 / ((p[2] * P[3][2]) - P[2][2]);"
    // "   float w = z * P[3][2];"
    // "   vec4 p_w = vec4(p.xy * w, z, w);"
    return p_world.slice<3>();
}

bool is_in_shadow(
    const vec<3>& cam_p, 
    const vec<3>& light_p, 
    const mat<4,4>& cam_view,
    const mat<4,4>& cam_proj,
    const mat<4,4>& light_view,
    const mat<4,4>& light_proj)
{
    return false;
}

/**
 * A test is nothing more than a stripped down C program
 * returning 0 is success. Use asserts to check for errors
 */
TEST
{
    { // ensure that a point can be retrieved after projection
        auto P = mat<4,4>::perspective(1, 100, M_PI / 2, 1.f);
        auto V = mat<4, 4>::look_at({0, 0, 10}, {0, 0, -1}, {0, 1, 0});

        auto p = vec<3>{0, 0, -20};
        auto p_prime = world_to_screen(P, V, p);

        // sanity check sanity check
        auto q = V * p;
        assert((V.invert() * q).is_near(p, 0.0001));

        auto p0 = screen_to_world(P, V, p_prime);

        assert(p0.is_near(p, 0.0001));
    }

    { // simple case with light behind viewer

        auto P = mat<4,4>::perspective(1, 100, M_PI / 2, 1.f);
        auto V = mat<4, 4>::look_at({0, 0, 10}, {0, 0, -1}, {0, 1, 0});

        auto p_world = vec<3>{0, 0, -10};
        auto p_light_screen = world_to_screen(P, V, p_world);
        auto p_cam_screen = world_to_screen(P, mat<4,4>::I(), p_world);

        // assert(is_in_shadow(
        //     p_cam_screen,
        //     p_light_screen,
        //     mat<4,4>::I(), P,
        //     V, P
        // ) == false);
    }


	return 0;
}
