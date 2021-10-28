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
    g::ai::mlp<3, 2> model;
    float throttle = 0, steering = 0;
    unsigned waypoint_index = 1;

    float time = 0;
    unsigned waypoints_reached = 0;

    car()
    {
        mass = 1;
        update_inertia_tensor();
    }

    inline float score() const { return waypoints_reached / time; }
    inline void reset()
    {
        waypoints_reached = 0;
        time = 0;
        position = { (float)(rand() % 10 - 5), 0, (float)(rand() % 10 - 5) };
        velocity *= 0;
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
        throttle = std::min<float>(1.f, std::max<float>(-0.5f, throttle));
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

        time += dt;
    }

    car breed(const car& mate, float mutation_rate=0.2f)
    {
        car child;

        uint8_t* my_model = (uint8_t*)&model;
        uint8_t* mate_model = (uint8_t*)&mate.model;
        uint8_t* child_model = (uint8_t*)&child.model;

        for (unsigned i = 0; i < sizeof(model); i++)
        {
            if(rand() % 2 == 0)
            {
                child_model[i] = my_model[i];
            }
            else
            {
                child_model[i] = mate_model[i];
            }

            for (unsigned j = 0; j < 8; j++)
            {
                float r = (rand() % 100) / 100.f;
                if (r < mutation_rate)
                {
                    child_model[i] ^= 1 << j;
                }
            }
        }

        return child;
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
    std::vector<mat4> waypoint_models;
    std::vector<vec<3>> waypoint_positions;

    float time = 0;
    unsigned generation = 0;

    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

        // glDisable(GL_CULL_FACE);
        cam.position = {0, 50, 4};
        plane = g::gfx::mesh_factory::plane({ 0, 1, 0 }, { 1000, 1000 });
        glPointSize(4);

        auto dt = (2 * M_PI)/20.f;
        for (float t = 0; t < 2 * M_PI; t += dt)
        {
            vec<3> p_0 = {100.f * sin(t), 0.f, 100.f * sin(t) * cos(t)};
            vec<3> p_1 = {100.f * sin(t + 0.1f), 0.f, 100.f * sin(t + 0.1f) * cos(t + 0.1f)};
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
        }

        for (unsigned i = 100; i--;)
        {
            car c = {};
            c.model.initialize();
            cars.push_back(c);
        }

        return true;
    }

    virtual void update(float dt)
    {
        if (dt > 1) { return; }

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

        cam.aspect_ratio = g::gfx::aspect();

        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        time += dt;

        plane.using_shader(assets.shader("basic_gui.vs+debug_normal.fs"))
        .set_camera(cam)
        ["u_model"].mat4(mat4::I())
        .draw<GL_TRIANGLE_FAN>();

        for (const auto& w : waypoint_models)
        {
            assets.geo("waypoint.obj").using_shader(assets.shader("basic_gui.vs+debug_normal.fs"))
                .set_camera(cam)
                ["u_model"].mat4(w)
                .draw<GL_TRIANGLES>();
        }

        for (auto& car : cars)
        {
            assets.geo("car.obj").using_shader(assets.shader("basic_gui.vs+debug_normal.fs"))
            .set_camera(cam)
            ["u_model"].mat4(car.transform())
            .draw<GL_TRIANGLES>();

            auto target = car.to_local(waypoint_positions[car.waypoint_index]  - car.position);
            auto x = vec<4>{target[0], target[2], car.velocity.magnitude(), 1, 1};
            auto y = car.model.evaluate(x);

            car.throttle = y[0];
            car.steering = y[1];

            car.update(dt, &cam);

            // advance to the next waypoint if the car made it
            if ((car.position - waypoint_positions[car.waypoint_index]).magnitude() < 8)
            {
                car.waypoint_index = (car.waypoint_index + 1) % waypoint_positions.size();
                car.waypoints_reached++;
            }
        }

        if (time > 30)
        {
            std::sort(cars.begin(), cars.end(), [](const car& a, const car& b){
                return a.score() > b.score();
            });

            std::vector<car> next_generation;

            std::cerr << "generation " << generation << " top scores" << std::endl;

            // take top 10 and add them to the next generation
            for (unsigned i = 0; i < 10; i++)
            {
                std::cerr << "#" << i << ": " << cars[i].score() << std::endl;
                cars[i].reset();
                next_generation.push_back(cars[i]);
            }

            // breed the top performers
            while(next_generation.size() < 100)
            {
                unsigned i = rand() % 10, j = rand() % 10;

                next_generation.push_back(cars[i].breed(cars[j], 0.01));
            }

            cars = next_generation;
            time = 0;
            generation++;
        }
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
