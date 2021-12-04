#include <g.h>
#include "g.ai.h"

#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

// #define DRAW_DEBUG_VECS
#define GENERATION_SIZE 1000

using mat4 = xmath::mat<4,4>;

struct car : g::dyn::rigid_body
{
    g::ai::mlp<6, 2> model;
    float throttle = 0, steering = 0;
    unsigned waypoint_index = 1;

    float time = 0;
    float waypoints_reached = 0;
    float cumulative_waypoints_reached = 0;

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

        waypoint_index = 1;
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

#ifdef DRAW_DEBUG_VECS
        glDisable(GL_DEPTH_TEST);
#endif

        for (unsigned i = 0; i < 4; i++)
        {
#ifdef DRAW_DEBUG_VECS
            g::gfx::debug::print(cam).color(colors[i]).point(position + to_global(positions[i]));
            g::gfx::debug::print(cam).color(colors[i]).ray(position + to_global(positions[i]), up());
            g::gfx::debug::print(cam).color(colors[i]).ray(position + to_global(positions[i]), to_global(lefts[i]));
#endif
            dyn_apply_local_force(positions[i], forwards[i] * throttle * 10);

            auto lin_vel_local = to_local(linear_velocity_at(positions[i]));
            auto lin_speed = lin_vel_local.magnitude();
            if (lin_speed > 0)
            {
                auto lin_vel_local_dir = lin_vel_local / lin_speed;
                auto ldv = lefts[i].dot(lin_vel_local_dir);
                auto fdv = forwards[i].dot(lin_vel_local_dir);
                auto drag = -lin_vel_local * (fabsf(fdv) * 0.01f + pow(fabsf(ldv), 1) * 0.95f);// * fdv * 1.f;// + -lefts[i] * sqrt(ldv);
#ifdef DRAW_DEBUG_VECS
                g::gfx::debug::print(cam).color({lin_vel_local[0], 0, lin_vel_local[2], 1}).ray(position + to_global(positions[i]), to_global(lin_vel_local));
                g::gfx::debug::print(cam).color({drag[0], 1, drag[2], 1}).ray(position + to_global(positions[i]), to_global(drag));
#endif
                dyn_apply_local_force(positions[i], drag);
            }
        }
#ifdef DRAW_DEBUG_VECS
        glEnable(GL_DEPTH_TEST);
#endif
        dyn_step(dt);

        time += dt;
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
    // g::ai::mlp<3, 2> brain;

    std::vector<car> cars;
    std::vector<mat4> waypoint_models;
    std::vector<vec<3>> waypoint_positions;
    std::vector<vec<3>> waypoint_dir;

    int skip = 1;
    float time = 0;
    unsigned generation = 0;
    unsigned best_car = 0;
    vec<3> subject, subject_velocity;

    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

        // glDisable(GL_CULL_FACE);
        cam.position = {0, 10, 50};
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
            waypoint_dir.push_back(f);
        }

        for (unsigned i = GENERATION_SIZE; i--;)
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

        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_MINUS) == GLFW_PRESS) skip = std::max<int>(1, skip - 1);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_EQUAL) == GLFW_PRESS) skip += 1;

        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // subject_velocity += ((cars[best_car].position + cars[best_car].velocity) - subject) * dt;
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
        for (const auto& w : waypoint_models)
        {
            waypoint_mesh.using_shader(shader)
            .set_camera(cam)
            ["u_model"].mat4(w)
            .draw<GL_TRIANGLES>();
        }

        auto& car_mesh = assets.geo("car.obj");
        for (unsigned i = 0; i < cars.size(); i++)
        {
            auto& car = cars[i];

            if (time > 10 && car.score() == 0) { continue; }

            if (i % skip == 0)
            car_mesh.using_shader(shader)
            .set_camera(cam)
            ["u_model"].mat4(car.transform())
            .draw<GL_TRIANGLES>();

            auto target = car.to_local(waypoint_positions[car.waypoint_index] - car.position);
            auto target_dir = car.to_local(waypoint_dir[car.waypoint_index]);
            auto vel_local = car.to_local(car.velocity);
            auto x = vec<7>{target[0], target[2], target_dir[0], target_dir[2], vel_local[0], vel_local[2], 1};
            auto y = car.model.evaluate(x);

            car.throttle = y[0];
            car.steering = y[1];

            car.update(dt, &cam);

            // advance to the next waypoint if the car made it
            auto dist_to_waypoint = (car.position - waypoint_positions[car.waypoint_index]).magnitude();
            if (dist_to_waypoint < 8)
            {
                car.waypoint_index = (car.waypoint_index + 1) % waypoint_positions.size();
                car.waypoints_reached += 1 / (dist_to_waypoint + 1.f);
                car.cumulative_waypoints_reached += 1 / (dist_to_waypoint + 1.f);
            }

            if (car.cumulative_waypoints_reached > cars[best_car].cumulative_waypoints_reached)
            {
                best_car = i;
            }
        }

        auto& king = cars[best_car];
        glDisable(GL_CULL_FACE);
        assets.geo("crown.obj").using_shader(shader)
        .set_camera(cam)
        ["u_model"].mat4(mat4::scale({2, 2, 2}) * mat4::translation(king.position + king.up() * 1.5 - king.forward() * 0.75))
        .draw<GL_TRIANGLES>();
        glEnable(GL_CULL_FACE);

        car_mesh.using_shader(shader)
        .set_camera(cam)
        ["u_model"].mat4(mat4::translation({100 * cos(time), 100 * cos(time), 100 * sin(time)}))
        .draw<GL_TRIANGLES>();

        if (time > 30)
        {
            std::vector<car> next_generation;
            g::ai::evolution::generation<car>(cars, next_generation, {});

            std::cerr << "generation " << generation << " top scores" << std::endl;
            for (unsigned i = 0; i < 10; i++)
            {
                std::cerr << "#" << i << ": " << cars[i].score() << std::endl;
                cars[i].reset();
                next_generation.push_back(cars[i]);
            }

            for (auto& car : next_generation) { car.reset(); }
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
