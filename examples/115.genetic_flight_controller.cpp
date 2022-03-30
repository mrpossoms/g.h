#include <g.h>
#include "g.ai.h"

#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

//#define DRAW_DEBUG_VECS
#define GENERATION_SIZE 3000

using mat4 = xmath::mat<4,4>;

float randf() { return (rand() % 2048 / 1024.f) - 1.f; }

vec<3> rand_unit()
{
    return vec<3>{ randf(), randf(), randf() }.unit();
}

struct drone : g::dyn::rigid_body
{
    vec<4> throttle = {};

    drone()
    {
        mass = 1;
        update_inertia_tensor();
    }

    inline vec<3> accelerometer() { return to_local(acceleration()); }

    inline vec<3> gyro_ypr()
    {
        return angular_velocity();
    }

    inline void update(float dt, g::game::camera* cam)
    {
        const vec<3> positions[4] = {
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
        position[1] -= deepest;
#ifdef DRAW_DEBUG_VECS
        glEnable(GL_DEPTH_TEST);
#endif
        dyn_apply_global_force(position, {0, -9.8, 0});

        // std::cerr << gyro_ypr().to_string() << std::endl;
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

    void reset()
    {
        position = { 0, 40, 0};
        angular_momentum = {};//rand_unit() * randf() * 0.1;
        linear_momentum = {};//rand_unit();
        orientation = quat<>::from_axis_angle({1, 0, 0}, M_PI - randf() * 1) * quat<>::from_axis_angle({0, 1, 0}, M_PI * randf() * 1);
    }
};

struct drone_agent
{
    g::ai::mlp<11, 4, 8> model;
    float reward = 0;

    drone drones[4];

    drone_agent()
    {
        model.initialize();
    }

    inline float score() const { return reward; }
    inline void reset()
    {
        reward = 0;
        for (unsigned i = 0; i < 4; i++)
        {
            drones[i].reset();
        }
    }

    void evaluate(float dt, g::game::camera_perspective* cam)
    {
        for (unsigned i = 0; i < 4; i++)
        {
            auto& drone = drones[i];

            drone.throttle = {};

            auto target_alt = 40.f;

            // do update
            {
                auto a = drone.accelerometer();
                auto w = drone.gyro_ypr();
                auto u = drone.up();
                auto v = drone.to_local(drone.velocity);
                auto alt = drone.position[1];
                auto x = vec<12>{w[0], w[1], w[2], u[0], u[1], u[2], v[0], v[1], v[2], alt, target_alt, 1};
                drone.throttle = model.evaluate(x);
            }

            drone.update(dt, cam);
            drone.dyn_step(dt);

            reward += (drone.up().dot({0, 1, 0})) - fabs(drone.position[1] - target_alt);            
        }


    }

    uint8_t* genome_buf() { return (uint8_t*)&model; }
    size_t genome_size() { return sizeof(model); }

};

struct my_core : public g::core
{
    g::asset::store assets;
    g::game::camera_perspective cam;
    g::gfx::mesh<g::gfx::vertex::pos_uv_norm> plane;

    std::vector<drone_agent> agents;

    float time_limit = 10;
    float time = 0;
    unsigned generation = 0;

    virtual bool initialize()
    {
        cam.position = {0, 10, 10};
        plane = g::gfx::mesh_factory::plane({ 0, 1, 0 }, { 1000, 1000 });
        glPointSize(4);

        for (unsigned i = GENERATION_SIZE; i--;)
        {
            drone_agent c = {};
            // c.angular_momentum = vec<3>{ 0, M_PI * 2, 0};
            // c.throttle[0] = c.throttle[1] = c.throttle[2] = c.throttle[3] = 1;
            c.reset();
            agents.push_back(c);
        }

        auto fd = open("best_fc.genome", O_RDONLY);
        if (fd >= 0)
        {
            read(fd, agents[0].genome_buf(), agents[0].genome_size());
        }

        return true;
    }

    virtual void update(float dt)
    {
        if (dt > 1) { return; }

        camera_controls(dt);

        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        time += dt;

        auto& shader = assets.shader("basic_gui.vs+debug_normal.fs");

        plane.using_shader(shader)
        .set_camera(cam)
        ["u_model"].mat4(mat4::I())
        .draw<GL_TRIANGLE_FAN>();


        auto& drone_mesh = assets.geo("quadrotor.obj");

        for (unsigned i = 0; i < agents.size(); i++)
        {
            agents[i].evaluate(dt, &cam);

            // drone.dyn_apply_local_force(drone.positions[rand() % 4], rand_unit());

            {
                drone_mesh.using_shader(shader)
                .set_camera(cam)
                ["u_model"].mat4(agents[i].drones[0].transform())
                .draw<GL_TRIANGLES>();
            }
        }


        if (time >= time_limit)
        {
            g::ai::evolution::generation_desc desc = {};

            desc.save_best = [&](const void* genome_buf, size_t genome_size)
            {
                auto fd = open("best_fc.genome", O_CREAT | O_WRONLY | O_TRUNC, 0666);

                if (fd >= 0)
                {
                    write(fd, genome_buf, genome_size);
                    close(fd);
                }
            };

            std::vector<drone_agent> next_generation;
            g::ai::evolution::generation<drone_agent>(agents, next_generation, desc);

            std::cerr << "generation " << generation << " top scores" << std::endl;
            for (unsigned i = 0; i < 10; i++)
            {
                std::cerr << "#" << i << ": " << agents[i].score() << std::endl;
                agents[i].reset();
                next_generation.push_back(agents[i]);
            }

            for (auto& drone : next_generation) { drone.reset(); }
            agents = next_generation;
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
    core.start({ argv[0], true, 1024, 768 });
// #endif

    return 0;
}
