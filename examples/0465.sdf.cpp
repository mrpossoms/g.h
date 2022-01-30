#include <g.h>

#include <algorithm>
#include <set>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

using mat4 = xmath::mat<4,4>;

float saturate(float x)
{
    return std::min<float>(1.f, std::max<float>(0, x));
}

float clamp(float x, float h, float l)
{
    return std::min<float>(h, std::max<float>(l, x));
}

struct my_core : public g::core
{
    g::gfx::shader basic_shader;
    g::asset::store assets;
    g::game::fps_camera cam;
    // g::gfx::mesh<g::gfx::vertex::pos_norm_tan> terrain;
    std::vector<int8_t> v[3];

    g::gfx::density_volume<g::gfx::vertex::pos_norm_tan>* terrain;

    g::game::sdf terrain_sdf;

    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

        // terrain = g::gfx::mesh_factory{}.empty_mesh<g::gfx::vertex::pos_norm_tan>();

        srand(time(NULL));

        {
            std::default_random_engine generator;
            std::uniform_int_distribution<int> distribution(-127,128);
            for (unsigned i = 2048; i--;)
            {
                v[0].push_back(distribution(generator));
                v[1].push_back(distribution(generator));
                v[2].push_back(distribution(generator));
            }
        }

        terrain_sdf = [&](const vec<3>& p) -> float {
            auto r = sqrtf(p.dot(p));
            auto base = r - 500.f;
            //d += g::gfx::noise::perlin(p * 9, v) * 0.01;
            // d += g::gfx::noise::perlin(p * 11, v) * 0.01;
            // d += g::gfx::noise::perlin(p * 3, v) * 0.1;

            // if (base <= 0) return base;

            // auto d = p[1] - 200;
            // d += g::gfx::noise::perlin(p*4.03, v[0]) * 0.125;
            // d += g::gfx::noise::perlin(p*1.96, v[1]) * 0.25;

            auto d = base;
            // d += g::gfx::noise::perlin(p*0.065, v[0]) * 10;
            // d += std::min<float>(0, g::gfx::noise::perlin(p*0.0334, v[1]) * 20);
            // d += std::min<float>(0, g::gfx::noise::perlin(p*0.0123, v[2]) * 40);
            // d = std::max<float>(d, -1);

            return d;
        };

        auto generator = [](const g::game::sdf& sdf, const vec<3>& pos) -> g::gfx::vertex::pos_norm_tan
        {
            g::gfx::vertex::pos_norm_tan v;

            v.position = pos;

            const float s = 1;
            vec<3> grad = {};
            vec<3> deltas[3][2] = {
                {{ s, 0, 0 }, { -s, 0, 0 }},
                {{ 0, s, 0 }, { 0, -s, 0 }},
                {{ 0, 0, s }, { 0,  0, -s }},
            };

            for (int j = 3; j--;)
            {
                vec<3> samples[2];
                samples[0] = pos + deltas[j][0];
                samples[1] = pos + deltas[j][1];
                grad[j] = sdf(samples[0]) - sdf(samples[1]);
            }

            v.normal = grad.unit();

            if (fabs(v.normal.dot({0, 1, 0})) > 0.999f)
            {
                v.tangent = vec<3>::cross(v.normal, {1, 0, 0});
            }
            else
            {
                v.tangent = vec<3>::cross(v.normal, {0, 1, 0});   
            }
        
            return v;
        };

        // std::vector<vec<3>> offsets = {{0, 0, 0}};
        std::vector<vec<3>> offsets;
        auto k = 1;
        for (float x = -k; x <= k; x++)
        for (float y = -k; y <= k; y++)
        for (float z = -k; z <= k; z++)
        {
            offsets.push_back({x, y, z});
        }

        terrain = new g::gfx::density_volume<g::gfx::vertex::pos_norm_tan>(terrain_sdf, generator, offsets);


        cam.position = {0, 525, 0};
        //glDisable(GL_CULL_FACE);

        return true;
    }

    virtual void update(float dt)
    {
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto speed = 4.0f;

        vec<3> down = -cam.position.unit();
        vec<3> feet = down * 2;
        cam.velocity += down * 1;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 10;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.velocity += cam.forward() * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.velocity += cam.forward() * -speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.velocity += cam.left() * -speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.velocity += cam.left() * speed;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_Q) == GLFW_PRESS) cam.d_roll(-dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_E) == GLFW_PRESS) cam.d_roll(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT) == GLFW_PRESS) cam.yaw += -dt;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_RIGHT) == GLFW_PRESS) cam.yaw += dt;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_UP) == GLFW_PRESS) cam.d_pitch(dt);
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_DOWN) == GLFW_PRESS) cam.d_pitch(-dt);

        
        auto p1 = cam.position + cam.velocity * dt;
        auto p0_d = terrain_sdf(cam.position);
        auto p1_d = terrain_sdf(p1);

        auto drag = cam.velocity * -0.01;
        auto w_bias = 0;
        cam.velocity += drag;

        if (p1_d <= 0)
        {
            auto n = g::game::normal_from_sdf(terrain_sdf, p1);
            auto w = p0_d / p0_d - p1_d;

            cam.velocity = cam.velocity - (n * (cam.velocity.dot(n) / n.dot(n)));
            auto friction = cam.velocity * -0.3;
            cam.velocity += friction;

            if (cam.velocity.magnitude() < 0.5) cam.velocity *= 0;

            for (; terrain_sdf(cam.position + cam.velocity * dt) <= 0; w += 0.1)
            {
                cam.position = (p1 * (1 - w) + (cam.position) * w);
            }

            // cam.position += cam.velocity * dt;
        }
        else
        {
            cam.position = p1;
        }

        cam.up = -down;
        if (fabs(cam.up.dot({0, 1, 0})) > 0.999f)
        {
            cam.right = vec<3>::cross(cam.up, {1, 0, 0});
        }
        else
        {
            cam.right = vec<3>::cross(cam.up, {0, 1, 0});
        }

        cam.update(dt, 0);
        terrain->update(cam);

        // draw terrain
        auto& wall = assets.tex("rock_wall.repeating.png");
        auto& ground = assets.tex("sand.repeating.png");
        auto& wall_normal = assets.tex("rock_wall_normal.repeating.png");
        auto& ground_normal = assets.tex("sand_normal.repeating.png");
        auto model = mat4::I();
        terrain->draw(cam, assets.shader("planet.vs+planet_color.fs"), [&](g::gfx::shader::usage& usage) {
            auto model = mat4::I();

            usage["u_wall"].texture(wall)
                 ["u_ground"].texture(ground)
                 ["u_wall_normal"].texture(wall_normal)
                 ["u_ground_normal"].texture(ground_normal)
                 ["u_model"].mat4(model)
                 ["u_time"].flt(t += dt * 0.01f);
        });
    }

    float t;
};

my_core core;

int main (int argc, const char* argv[])
{
    core.start({ "046.heightmap", true, 512, 512 });

    return 0;
}
