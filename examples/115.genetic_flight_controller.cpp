#include <g.h>
#include "g.ai.h"

#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

//#define DRAW_DEBUG_VECS
#define GENERATION_SIZE 2000

using mat4 = xmath::mat<4,4>;

float randf() { return (rand() % 2048 / 1024.f) - 1.f; }

vec<3> rand_unit()
{
    return vec<3>{ randf(), randf(), randf() }.unit();
}

struct drone : g::dyn::rigid_body
{
    g::ai::mlp<9, 4, 1> model;
    vec<4> throttle = {};
    float last_dist_to_waypoint = 30;
    unsigned waypoint_index = 1;
    float reward = 0;
    float energy = 10;

    drone()
    {
        mass = 1;
        update_inertia_tensor();
        model.initialize();
    }

    inline float score() const { return reward; }
    inline void reset()
    {
        reward = 0;
        auto t = (2*M_PI / 20.0f) * 0.1f;
        position = {100.f * sinf(t), 40.f + cosf(t) * 10.f, 100.f * sinf(t) * cosf(t)};
        position += { randf() * 10, randf(), randf() * 10};
        angular_momentum = {};//rand_unit() * randf() * 0.1;
        linear_momentum = {};//rand_unit();
        orientation = /*quat<>::from_axis_angle({1, 0, 0}, M_PI) **/ quat<>::from_axis_angle({0, 1, 0}, M_PI * randf());
        waypoint_index = 1;
        energy = 10;
        last_dist_to_waypoint = 30;
    }

    inline vec<3> accelerometer() { return to_local(acceleration()); }

    inline vec<3> gyro_ypr()
    {
        return angular_velocity();
    }

    inline void update(float dt, g::game::camera* cam)
    {
        vec<3> positions[4] = {
            {-1, 0, 0}, // rear left
            { 1, 0, 0}, // rear right
            { 0, 0, -1}, // front left
            { 0, 0,  1}, // front right
        };

        vec<4> colors[4] = {
            {0.5f, 0, 0, 1},
            {1.0f, 0, 0, 1},
            {0.5f, 0, 1, 1},
            {1.0f, 0, 1, 1},
        };

#ifdef DRAW_DEBUG_VECS
        glDisable(GL_DEPTH_TEST);
#endif
        float deepest = 0;
        for (unsigned i = 0; i < 4; i++)
        {
            auto lin_vel_i = linear_velocity_at(positions[i]);

#ifdef DRAW_DEBUG_VECS
            g::gfx::debug::print(cam).color(colors[i]).point(position + to_global(positions[i]));
            g::gfx::debug::print(cam).color(colors[i]).ray(position + to_global(positions[i]), up());
            g::gfx::debug::print(cam).color({1, 0, 0, 1}).ray(position + to_global(positions[i]), (lin_vel_i));
#endif
            // dyn_apply_local_force(positions[i], vec<3>{0, 1, 0} * throttle * 10);

            if ((position + to_global(positions[i]))[1] < 0)
            {
                auto f = -(lin_vel_i * mass * 0.25f) / dt;
                dyn_apply_local_force(positions[i], to_local(f));

                auto penetration = position[1] + to_global(positions[i])[1];
                if (penetration < deepest) { deepest = penetration; }
            }

            float thrust = std::min<float>(1.f, std::max<float>(0.f, throttle[i])) * (20 * 0.25);
            // energy -= throttle[i] * dt;
            dyn_apply_local_force(positions[i], vec<3>{0, thrust, 0});
        }
        energy -= dt;
        position[1] -= deepest;
#ifdef DRAW_DEBUG_VECS
        glEnable(GL_DEPTH_TEST);
#endif
        dyn_apply_global_force(position, {0, -9.8, 0});

        // std::cerr << gyro_ypr().to_string() << std::endl;
    }

    uint8_t* genome_buf() { return (uint8_t*)&model; }
    size_t genome_size() { return sizeof(model); }


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

    std::vector<drone> drones;
    std::vector<mat4> waypoint_models;
    std::vector<vec<3>> waypoint_positions;
    std::vector<vec<3>> waypoint_dir;

    float time_limit = 10;
    float last_record = 0;
    float time = 0;
    unsigned generation = 0;
    unsigned best_drone = 0;
    vec<3> subject, subject_velocity;

    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

        // glDisable(GL_CULL_FACE);
        cam.position = {0, 10, 10};
        plane = g::gfx::mesh_factory::plane({ 0, 1, 0 }, { 1000, 1000 });
        glPointSize(4);

        for (unsigned i = GENERATION_SIZE; i--;)
        {
            drone c = {};
            // c.angular_momentum = vec<3>{ 0, M_PI * 2, 0};
            // c.throttle[0] = c.throttle[1] = c.throttle[2] = c.throttle[3] = 1;
            c.reset();
            drones.push_back(c);
        }

        auto dt = (2 * M_PI)/20.f;
        for (float t = 0; t < 2 * M_PI; t += dt)
        {
            vec<3> p_0 = {100.f * sinf(t), 40.f + cosf(t) * 10.f, 100.f * sinf(t) * cosf(t)};
            vec<3> p_1 = {100.f * sinf(t + 0.1f), 40.f + cosf(t + 0.1) * 10.f, 100.f * sinf(t + 0.1f) * cosf(t + 0.1f)};
            waypoint_positions.push_back(p_0);

            auto f = (p_1 - p_0).unit();
            auto l = vec<3>::cross(f, {0, 1, 0});
            l *= 2; f *= 2;
            auto rotate = mat4{
                { l[0], l[1], l[2],  0 },
                {    0,   -2,    0,  0 },
                { f[0], f[1], f[2],  0 },
                {    0,    0,    0,  1 },
            };

            waypoint_models.push_back(rotate * mat4::translation(p_0 + vec<3>{0, 2, 0}));
            waypoint_dir.push_back(f);
        }

        return true;
    }

    virtual void update(float dt)
    {
        if (dt > 1) { return; }

        camera_controls(dt);

        cam.aspect_ratio = g::gfx::aspect();

        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // subject_velocity += ((drones[best_drone].position + drones[best_drone].velocity) - subject) * dt;
        // subject_velocity *= 0.95f;
        // subject += subject_velocity * dt;
        // auto forward = (subject - cam.position).unit();
        // auto left = vec<3>::cross(forward, {0, -1, 0});
        // cam.orientation = quat<>::view(forward, vec<3>::cross(forward, left));
        // cam.position += ((subject + vec<3>{10, 20, 10}) - cam.position) * dt;

        time += dt;

        auto& shader = assets.shader("basic_gui.vs+debug_normal.fs");

        plane.using_shader(shader)
        .set_camera(cam)
        ["u_model"].mat4(mat4::I())
        .draw<GL_TRIANGLE_FAN>();

        auto& waypoint_mesh = assets.geo("waypoint.obj");
        for (unsigned i = 0; i < waypoint_models.size(); i++)
        {
            waypoint_mesh.using_shader(shader)
            .set_camera(cam)
            ["u_model"].mat4(waypoint_models[i])
            .draw<GL_TRIANGLES>();

            auto dir = waypoint_positions[(i + 1) % waypoint_models.size()] - waypoint_positions[i];
            g::gfx::debug::print(&cam).color({ i / (float)waypoint_positions.size(), 0, 0, 1}).ray(waypoint_positions[i], dir);

        }
        g::gfx::debug::print(&cam).color({ 1, 0, 0, 1}).point(waypoint_positions[1]);


        auto all_dead = true;
        auto& drone_mesh = assets.geo("quadrotor.obj");
        for (unsigned i = 0; i < drones.size(); i++)
        {
            auto& drone = drones[i];
            drone.update(dt, &cam);
            drone.throttle = {};

            if (drone.position[1] < 5) { drone.energy = 0; }

            if (drone.energy > 0)
            {
                auto a = drone.accelerometer();
                auto w = drone.gyro_ypr();
                auto u = drone.up();
                auto v = drone.to_local(drone.velocity);
                auto g = drone.to_local(waypoint_positions[drone.waypoint_index] - drone.position);
                auto x = vec<10>{w[0], w[1], w[2], v[0], v[1], v[2], g[0], g[1], g[2], 1};
                drone.throttle = drone.model.evaluate(x);

                all_dead = false;
            }

            // if (i % skip == 0)
            drone.dyn_step(dt);

            auto dist_to_waypoint = (drone.position - waypoint_positions[drone.waypoint_index]).magnitude();
            if (dist_to_waypoint < 6)
            {
                drone.waypoint_index = (drone.waypoint_index + 1) % waypoint_positions.size();
                // drone.reward += 100 / (dist_to_waypoint + 1.f);
                drone.energy += 5;
                drone.last_dist_to_waypoint = (drone.position - waypoint_positions[drone.waypoint_index]).magnitude();
            }

            drone.reward += (drone.last_dist_to_waypoint - dist_to_waypoint) * (drone.up().dot({0, 1, 0}) + 1);
            drone.last_dist_to_waypoint = dist_to_waypoint;

            // drone.reward += (drone.up().dot({0, 1, 0})) / (1 + (waypoint_positions[drone.waypoint_index] - drone.position).magnitude());

            //if (i % 100 == 0)
            {
                drone_mesh.using_shader(shader)
                .set_camera(cam)
                ["u_model"].mat4(drones[i].transform())
                .draw<GL_TRIANGLES>();
            }
        }


        // auto& king = drones[best_drone];
        // glDisable(GL_CULL_FACE);
        // assets.geo("crown.obj").using_shader(shader)
        // .set_camera(cam)
        // ["u_model"].mat4(mat4::scale({2, 2, 2}) * mat4::translation(king.position + king.up() * 1.5 - king.forward() * 0.75))
        // .draw<GL_TRIANGLES>();
        // glEnable(GL_CULL_FACE);

        if (all_dead)
        {
            std::vector<drone> next_generation;
            g::ai::evolution::generation<drone>(drones, next_generation, {});

            // auto delta = next_generation[0].reward - last_record;
            // if (delta > 0)
            // {
            //     time_limit += delta;
            //     last_record = next_generation[0].reward;
            // }

            std::cerr << "generation " << generation << " top scores" << std::endl;
            for (unsigned i = 0; i < 10; i++)
            {
                std::cerr << "#" << i << ": " << drones[i].score() << std::endl;
                drones[i].reset();
                next_generation.push_back(drones[i]);
            }

            for (auto& drone : next_generation) { drone.reset(); }
            drones = next_generation;
            time = 0;
            generation++;
        }
    }

    void camera_controls(float dt)
    {
        const auto speed = 8.0f;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.position += cam.forward() * dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.position += cam.forward() * -dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.position += cam.left() * -dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.position += cam.left() * dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) cam.position += cam.up() * dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cam.position += cam.up() * -dt * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_Q) == GLFW_PRESS) cam.d_roll(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_E) == GLFW_PRESS) cam.d_roll(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.d_yaw(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.d_yaw(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);

        // if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_MINUS) == GLFW_PRESS) skip = std::max<int>(1, skip - 1);
        // if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_EQUAL) == GLFW_PRESS) skip += 1;

    }
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
