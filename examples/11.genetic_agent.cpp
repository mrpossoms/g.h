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
        mass = 1000;
        update_inertia_tensor();
    }


    inline void steer(float a)
    {
        if (velocity.magnitude() > 0)
        {
            float steer_strength = (velocity.magnitude() * a) / velocity.magnitude();
            steer_strength = std::min<float>(1.f, steer_strength);
            auto force = left() * steer_strength;
            dyn_apply_force(forward(), force);
        }
    }

    inline void update(float dt, g::game::camera* cam)
    {
        throttle = std::min(1.f, std::max(-1.f, throttle));
        steering = std::min(1.f, std::max(-1.f, steering));
        auto steering_angle = steering * M_PI / 4;
        auto steering_q = quat<>::from_axis_angle(up(), steering_angle);

        { // forces from steering and throttle
            auto accel = vec<3>{ 0, 0, throttle * 1000 };
            dyn_apply_force(forward() * -0.5f, accel);
            auto powered_steer = steering_q.rotate(accel);
            dyn_apply_force(forward() * 0.5f, powered_steer);
        }

        { // tire drag (causes redirection)
            auto wheel_left = steering_q.rotate(left());

            auto left_dot_vel = left().dot(velocity);
            auto skid_force = -left() * (1000 * (pow(left_dot_vel, 3) + left_dot_vel));

            glDisable(GL_DEPTH_TEST);
            g::gfx::debug::print(cam).color({ 0, 0, 0, 1 }).ray(position - left() + forward() * 2, wheel_left * 2);
            g::gfx::debug::print(cam).color({ 0, 0, 0, 1 }).ray(position - left() - forward() * 2, wheel_left * 2);
            g::gfx::debug::print(cam).color({ 0, 0, 1, 1 }).ray(position - forward(), skid_force);

            g::gfx::debug::print(cam).color({ 0, 0, 0, 1 }).ray(position, forward());
            g::gfx::debug::print(cam).color({ 1, 0, 0, 1 }).ray(position, velocity);

            glEnable(GL_DEPTH_TEST);

            dyn_apply_force({}, skid_force);
            // dyn_apply_force(forward() * 0.5f, steer_force * 1000);

            dyn_apply_force(forward() * -2.f, left() * steering * -1000);
            dyn_apply_force(forward() * 2.f, left() * steering * 1000);

        }

        { // parasitic forces
            auto v = velocity.magnitude();
            auto drag = vec<3>{ 0, 0, -(pow(v, 2.f) + v * 2) };
            dyn_apply_force({ 0, 0, 0 }, drag);
        }

        //auto tire_forces = left() * left().dot(velocity) * pow(velocity.magnitude() * 10, 2);
        //dyn_apply_force({ 0, 0, 0 }, tire_forces);
        angular_momentum -= angular_momentum * dt;

        //velocity = forward() * forward().dot(velocity);

        dyn_step(dt);
    }

    inline vec<3> local_velocity() { return orientation.inverse().rotate(velocity); }

    inline vec<3> forward() { return orientation.inverse().rotate({0, 0, 1}); }

    inline vec<3> up() { return orientation.inverse().rotate({ 0, 1, 0 }); }

    inline vec<3> left() { return orientation.inverse().rotate({ 1, 0, 0 }); }

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
        cam.position = {0, 10, 4};
        plane = g::gfx::mesh_factory::plane({ 0, 1, 0 }, { 100, 100 });

        cars.push_back({});

        return true;
    }

    virtual void update(float dt)
    {
        const auto speed = 4.0f;
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
    core.start({ "11.genetic_agent", true, 512, 512 });
// #endif

    return 0;
}
