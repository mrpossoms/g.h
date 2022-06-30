#include ".test.h"
#include "g.h"

using namespace xmath;

template <size_t R, size_t C>
void raymarch_sphere_scene(mat<R, C>& dst, const mat<4,4>& proj, const mat<4, 4>& view)
{
    auto sdf = [&](const vec<3>& p) -> float  {
        auto plane = p.y;
        auto sphere = (p - vec<3>{0, 1, 0}).magnitude() - 1;
        return std::min<float>(plane, sphere);
    };

    auto v_view_rotation = mat<3,3>(
        view[0].slice<3>().unit(),
        view[1].slice<3>().unit(),
        view[2].slice<3>().unit()
    ).invert();

    for (unsigned r = 0; r < R; r++)
    for (unsigned c = 0; c < C; c++)
    {
        auto u = (c / 64.f) - 1.f;
        auto v = (r / 64.f) - 1.f; 
        vec3 ray = proj * vec<3>(-u, -v, 1.0);
        //ray /= ray.w;

        vec3 d = v_view_rotation * ray.unit();
        float z = 0;
        vec3 o = view * vec<3>{};

        for (unsigned t = 10; t--;)
        {
            auto dist = sdf(o + d * z);

            if (dist > 0.01f) { z += d; }
            else if(t)
            else
            {

            }
        }
    }
}

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

    auto a = proj[3][2];
    auto b = proj[2][2];

    auto _z = -1 / ((p[2] * a) - b);
    // undo the perspective divide and projection
    auto p_view = xmath::vec<4>{
        p[0] * (_z * a),
        p[1] * (_z * a),
        _z,
        1//{_z * a}
    };
    std::cerr << "screen " << p.to_string() << std::endl;

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

    auto cam_w = screen_to_world(cam_proj, cam_view, cam_p);
    auto light_w = screen_to_world(light_proj, light_view, light_p);

    return (cam_w - light_w).magnitude() > 0.001f;
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

         assert(is_in_shadow(
             p_cam_screen,
             p_light_screen,
             mat<4,4>::I(), P,
             V, P
         ) == false);
    }

    { // simple case with light behind viewer, and the viewer occludes

        auto P = mat<4,4>::perspective(1, 100, M_PI / 2, 1.f);
        auto V = mat<4, 4>::look_at({0, 0, 10}, {0, 0, -1}, {0, 1, 0});

        auto p_world = vec<3>{0, 0, -10};
        auto p_light_screen = world_to_screen(P, V, { 0, 0, 0 });
        auto p_cam_screen = world_to_screen(P, mat<4,4>::I(), p_world);

         assert(is_in_shadow(
             p_cam_screen,
             p_light_screen,
             mat<4,4>::I(), P,
             V, P
         ) == true);
    }

    { // less trivial setup, point in shadow

        auto light_pos = vec<3>{ 10, 0, 10 };
        auto p_world = vec<3>{0, 1, 0};
        auto o_world = (light_pos + p_world) / 2;
        
        auto P = mat<4,4>::perspective(1, 100, M_PI / 2, 1.f);
        auto cam_V = mat<4, 4>::look_at({0, 0, 10}, {0, 0, -1}, {0, 1, 0});
        auto light_V = mat<4, 4>::look_at(light_pos, { 0, 0, -1 }, { 0, 1, 0 });

        auto p_light_screen = world_to_screen(P, light_V, o_world);
        auto p_cam_screen = world_to_screen(P, cam_V, p_world);

         assert(is_in_shadow(
             p_cam_screen,
             p_light_screen,
             cam_V, P,
             light_V, P
         ) == true);
    }

    { // less trivial setup, point in light

        auto light_pos = vec<3>{ 10, 0, 10 };
        auto p_world = vec<3>{0, 1, 0};

        auto P = mat<4,4>::perspective(1, 100, M_PI / 2, 1.f);
        auto cam_V = mat<4, 4>::look_at({0, 0, 10}, {0, 0, -1}, {0, 1, 0});
        auto light_V = mat<4, 4>::look_at(light_pos, { 0, 0, -1 }, { 0, 1, 0 });

        auto p_light_screen = world_to_screen(P, light_V, p_world);
        auto p_cam_screen = world_to_screen(P, cam_V, p_world);

         assert(is_in_shadow(
             p_cam_screen,
             p_light_screen,
             cam_V, P,
             light_V, P
         ) == false);
    }

    { // fuzzing test where light and camera are randomly positioned about the unit sphere
      // and randomly occluded or not
        auto P = mat<4,4>::perspective(0.1f, 10, M_PI / 2, 1.f);

        for (unsigned i = 0; i < 100; i++)
        {
            auto cam_pos = vec<3>{ RAND_F, RAND_F, RAND_F }.unit();
            auto light_pos = vec<3>{ RAND_F, RAND_F, RAND_F }.unit();
  
            auto cam_point = vec<3>{};
            auto light_point = vec<3>{};

            auto point_in_shadow = (RAND_F < 0.5);

            if (point_in_shadow)
            {
                light_point = (light_point + light_pos) * std::max<float>(0.11f, RAND_F);
            }

            auto cam_view = mat<4, 4>::look_at(cam_pos, (cam_pos - cam_point).unit(), { 0, 1, 0 });
            auto light_view = mat<4, 4>::look_at(light_pos, (light_pos - light_point).unit(), { 0, 1, 0 });

            auto p_light_screen = world_to_screen(P, light_view, light_point);
            auto p_cam_screen = world_to_screen(P, cam_view, cam_point);

            assert(is_in_shadow(
                p_cam_screen,
                p_light_screen,
                cam_view, P,
                light_view, P
            ) == point_in_shadow);
        }

    }

	return 0;
}
