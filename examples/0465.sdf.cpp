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
    std::vector<int8_t> v[3];

    g::gfx::density_volume<g::gfx::vertex::pos_norm_tan>* terrain;

    g::game::sdf terrain_sdf;

    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

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
            auto base = r - 1000.f;
            //d += g::gfx::noise::perlin(p * 9, v) * 0.01;
            // d += g::gfx::noise::perlin(p * 11, v) * 0.01;
            // d += g::gfx::noise::perlin(p * 3, v) * 0.1;

            // if (base <= 0) return base;

            // auto d = p[1] - 200;
            // d += g::gfx::noise::perlin(p*4.03, v[0]) * 0.125;
            // d += g::gfx::noise::perlin(p*1.96, v[1]) * 0.25;

            auto d = base;
            d += g::gfx::noise::perlin(p*0.065, v[0]);
            d += std::min<float>(0, g::gfx::noise::perlin(p*0.0234, v[1]) * 40);
            
            d += g::gfx::noise::perlin(p*0.0123, v[2]) * 80;
           

            return d;
        };

        auto generator = [](const g::game::sdf& sdf, const vec<3>& pos) -> g::gfx::vertex::pos_norm_tan
        {
            g::gfx::vertex::pos_norm_tan v;

            v.position = pos;

            const float s = 1;
            v.normal = normal_from_sdf(sdf, pos, s);

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

        std::vector<vec<3>> offsets;
        auto k = 1;
        for (float x = -k; x <= k; x++)
        for (float y = -k; y <= k; y++)
        for (float z = -k; z <= k; z++)
        {
            offsets.push_back({x, y, z});
        }

        terrain = new g::gfx::density_volume<g::gfx::vertex::pos_norm_tan>(terrain_sdf, generator, offsets);
        terrain->scale = 200;
        terrain->depth = 4;

        //cam.position = {218.369263, 968.625732, -140.036774};
        cam.position = { 0, 1100, 0 };
        cam.foot_offset = { 0, -1.5, 0 };

        cam.on_input = [](fps_camera& cam, float dt){
            static double xlast, ylast;
            float sensitivity = 0.5f;
            double xpos = 0, ypos = 0;
            auto mode = glfwGetInputMode(g::gfx::GLFW_WIN, GLFW_CURSOR);

            if (GLFW_CURSOR_DISABLED == mode)
            {
                glfwGetCursorPos(g::gfx::GLFW_WIN, &xpos, &ypos);
            }

            if (glfwGetInputMode(g::gfx::GLFW_WIN, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
            if (xlast != 0 || ylast != 0)
            {
                auto dx = xpos - xlast;
                auto dy = ypos - ylast;
                cam.pitch += (-dy * dt * sensitivity);
                cam.yaw += (dx * dt * sensitivity);
            }

            xlast = xpos; ylast = ypos;

            auto speed = cam.speed;
            speed *= cam.touching_surface ? 1 : 0.3;
            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 5;
            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_W) == GLFW_PRESS) cam.velocity += cam.body_forward() * speed;
            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_S) == GLFW_PRESS) cam.velocity += cam.body_forward() * -speed;
            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_A) == GLFW_PRESS) cam.velocity += cam.body_left() * speed;
            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_D) == GLFW_PRESS) cam.velocity += cam.body_left() * -speed;
            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_SPACE) == GLFW_PRESS) cam.velocity += cam.body_up() * 5 * cam.touching_surface;
            if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetInputMode(g::gfx::GLFW_WIN, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        
            if (glfwGetMouseButton(g::gfx::GLFW_WIN, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
            {
                glfwSetInputMode(g::gfx::GLFW_WIN, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        };

        //glDisable(GL_CULL_FACE);

        glfwSetInputMode(g::gfx::GLFW_WIN, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        return true;
    }

    virtual void update(float dt)
    {
        glClearColor(0.5, 0.5, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        g::dyn::cd::sdf_collider ground_collider(terrain_sdf);

        vec<3> down = -cam.position.unit();
        cam.gravity = down * 9.8;
        cam.pre_update(dt, 0);

        auto intersections = cam.intersections(ground_collider, 1);
        cam.touching_surface = intersections.size() > 0;

        if (cam.touching_surface)
        {
            g::dyn::cr::resolve_linear<fps_camera>(cam, intersections);
        }

        cam.update(dt, 0);

        glPointSize(4);
        g::gfx::debug::print(&cam).color({ 0, 1, 0, 1 }).ray(cam.position, cam.forward());
        g::gfx::debug::print(&cam).color({ 1, 0, 0, 1 }).ray(cam.position + cam.forward(), cam.left());
        g::gfx::debug::print(&cam).color({ 0, 0, 1, 1 }).point(cam.position + cam.forward());

        g::gfx::debug::print(&cam).color({ 1, 0, 0, 1 }).ray(vec<3>{ 0, 0, 0 }, { 1000, 0, 0 });
        g::gfx::debug::print(&cam).color({ 0, 1, 0, 1 }).ray(vec<3>{ 0, 0, 0 }, { 0, 1000, 0 });
        g::gfx::debug::print(&cam).color({ 0, 0, 1, 1 }).ray(vec<3>{ 0, 0, 0 }, { 0, 0, 1000 });

        terrain->update(cam);


        // grab textures for terrain
        auto& wall = assets.tex("rock_wall.repeating.png");
        auto& ground = assets.tex("sand.repeating.png");
        auto& wall_normal = assets.tex("rock_wall_normal.repeating.png");
        auto& ground_normal = assets.tex("sand_normal.repeating.png");
        auto model = mat4::I();

        // draw terrain
        terrain->draw(cam, assets.shader("planet.vs+planet_color.fs"), [&](g::gfx::shader::usage& usage) {
            auto model = mat4::I();

            usage["u_wall"].texture(wall)
                 ["u_ground"].texture(ground)
                 ["u_wall_normal"].texture(wall_normal)
                 ["u_ground_normal"].texture(ground_normal)
                 ["u_model"].mat4(model)
                 ["u_time"].flt(t += dt * 0.01f);
        });
    
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_ESCAPE) == GLFW_PRESS) { this->running = false; }
    }

    float t;
};

my_core core;

int main (int argc, const char* argv[])
{
    core.start({ "046.heightmap", true, 512, 512 });

    return 0;
}
