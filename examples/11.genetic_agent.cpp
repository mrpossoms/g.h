#include <g.h>
#include "g.ai.h"

#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

using mat4 = xmath::mat<4,4>;

struct car : g::dyn::rigid_body
{
    g::ai::mlp<1, 2> model;
    float throttle = 0, steering = 0;

    car()
    {
        mass = 1;
        update_inertia_tensor();
    }


    inline void steer(float a)
    {
        if (velocity.magnitude() > 0)
        {
            float steer_strength = (velocity.magnitude() * a) / velocity.magnitude();
            steer_strength = std::min<float>(1.f, steer_strength);
            auto force = left() * steer_strength;
            dyn_apply_local_force(forward(), force);
        }
    }

    inline void update(float dt, g::game::camera* cam)
    {
        throttle = std::min<float>(1.f, std::max<float>(-1.f, throttle));
        steering = std::min<float>(1.f, std::max<float>(-1.f, steering));
        auto steering_angle = steering * M_PI / 4;
        auto steering_q = quat<>::from_axis_angle(up(), steering_angle);

        vec<3> positions[4] = {
            {-1, 0, -2}, // rear left
            { 1, 0, -2}, // rear right
            {-1, 0,  2}, // front left
            { 1, 0,  2}, // front right
        };

        vec<3> forwards[4] = {
            {0, 0, 1}, // rear left
            {0, 0, 1}, // rear right
            steering_q.rotate({0, 0, 1}), // front left
            steering_q.rotate({0, 0, 1}), // front right
        };

        vec<3> lefts[4] = {
            {-1, 0, 0}, // rear left
            {+1, 0, 0}, // rear right
            steering_q.rotate({-1, 0, 0}), // front left
            steering_q.rotate({+1, 0, 0}), // front right
        };

        vec<4> colors[4] = {
            {0.5f, 0, 0, 1},
            {1.0f, 0, 0, 1},
            {0.5f, 0, 1, 1},
            {1.0f, 0, 1, 1},
        };

        glDisable(GL_DEPTH_TEST);

        for (unsigned i = 0; i < 4; i++)
        {
            g::gfx::debug::print(cam).color(colors[i]).point(position + to_global(positions[i]));
            g::gfx::debug::print(cam).color(colors[i]).ray(position + to_global(positions[i]), up());
            g::gfx::debug::print(cam).color(colors[i]).ray(position + to_global(positions[i]), to_global(lefts[i]));
            dyn_apply_local_force(positions[i], forwards[i] * throttle * 10);

            auto lin_vel_local = to_local(linear_velocity_at(positions[i]));
            auto lin_speed = lin_vel_local.magnitude();
            if (lin_speed > 0)
            {
                auto lin_vel_local_dir = lin_vel_local / lin_speed;
                auto ldv = lefts[i].dot(lin_vel_local_dir);
                auto fdv = forwards[i].dot(lin_vel_local_dir);
                g::gfx::debug::print(cam).color({lin_vel_local[0], 0, lin_vel_local[2], 1}).ray(position + to_global(positions[i]), to_global(lin_vel_local));

                auto drag = -lin_vel_local * (fabsf(fdv) * 0.01f + pow(fabsf(ldv), 1) * 0.95f);// * fdv * 1.f;// + -lefts[i] * sqrt(ldv);
                g::gfx::debug::print(cam).color({drag[0], 1, drag[2], 1}).ray(position + to_global(positions[i]), to_global(drag));
                dyn_apply_local_force(positions[i], drag);
            }
        }
        glEnable(GL_DEPTH_TEST);

        dyn_step(dt);
    }

    inline vec<3> forward() { return orientation.inverse().rotate({0, 0, 1}); }

    inline vec<3> up() { return orientation.inverse().rotate({ 0, 1, 0 }); }

    inline vec<3> left() {
        return orientation.inverse().rotate({ 1, 0, 0 });
    }

    inline mat4 transform()
    {
        return orientation.to_matrix() * mat4::translation(position);
    }
};

struct my_core : public g::core
{
    g::asset::store assets;
    g::game::camera_perspective cam;
    g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;
    // g::ai::mlp<3, 2> brain;

    std::vector<car> cars;


    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

        // glDisable(GL_CULL_FACE);
        cam.position = {0, 50, 4};
        plane = g::gfx::mesh_factory::plane({ 0, 1, 0 }, { 100, 100 });
        glPointSize(4);
        cars.push_back({});

        cars[0].angular_momentum = {0, 1, 0};

        return true;
    }

    virtual void update(float dt)
    {
        const auto speed = 8.0f;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.position += cam.forward() * dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.position += cam.forward() * -dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.position += cam.left() * -dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.position += cam.left() * dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_Q) == GLFW_PRESS) cam.d_roll(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_E) == GLFW_PRESS) cam.d_roll(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) { cars[0].velocity *= 0; cars[0].angular_momentum *= 0; }
        //cam.position = cam.position * (1 - dt) + (cars[0].position + cars[0].forward() * -4 + cars[0].up() * 2) * dt;
        //cam.orientation = (cam.orientation * (1 - dt) + cars[0].orientation.inverse() * dt).unit();
        //cam.look_at(cars[0].position);

        cars[0].throttle = 0;
        cars[0].steering = 0;

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_I) == GLFW_PRESS)
        {
            cars[0].throttle = 1;
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_K) == GLFW_PRESS)
        {
            cars[0].throttle = -1;
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_J) == GLFW_PRESS)
        {
            cars[0].steering = 1;
        }

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_L) == GLFW_PRESS)
        {
            cars[0].steering = -1;
        }

        cam.aspect_ratio = g::gfx::aspect();

        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        t += dt;

        plane.using_shader(assets.shader("basic_gui.vs+debug_normal.fs"))
        .set_camera(cam)
        ["u_model"].mat4(mat4::I())
        .draw<GL_TRIANGLE_FAN>();

        assets.geo("car.obj").using_shader(assets.shader("basic_gui.vs+debug_normal.fs"))
        .set_camera(cam)
        ["u_model"].mat4(cars[0].transform())
        .draw<GL_TRIANGLES>();

        assets.geo("waypoint.obj").using_shader(assets.shader("basic_gui.vs+debug_normal.fs"))
            .set_camera(cam)
            ["u_model"].mat4(mat4::I())
            .draw<GL_TRIANGLES>();

        for (auto& c : cars)
        {
            c.update(dt, &cam);
        }
    }

    float t;
};

my_core core;

// void main_loop() { core.tick(); }

int main (int argc, const char* argv[])
{

// #ifdef __EMSCRIPTEN__
//  core.running = false;
//  core.start({ "04.basic_draw", true, 512, 512 });
//  emscripten_set_main_loop(main_loop, 144, 1);
// #else
    core.start({ "11.genetic_agent", true, 1024, 768 });
// #endif

    return 0;
}
