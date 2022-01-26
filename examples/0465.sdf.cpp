#include <g.h>

#include <algorithm>
#include <set>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

using mat4 = xmath::mat<4,4>;

template<typename V>
struct terrain_volume
{
    struct terrain_block
    {
        g::gfx::mesh<V> mesh;

        vec<3, int> index;
        vec<3> bounding_box[2];
        uint8_t vertex_case = 0;
        bool regenerating = false;

        inline bool contains(const vec<3>& pos) const
        {
            return pos[0] > bounding_box[0][0] && pos[0] <= bounding_box[1][0] &&
                   pos[1] > bounding_box[0][1] && pos[1] <= bounding_box[1][1] &&
                   pos[2] > bounding_box[0][2] && pos[2] <= bounding_box[1][2];
        }

        inline bool contains(const vec<3, int> idx) const
        {
            return index == idx;
        }
    };

    std::vector<terrain_block> blocks;
    std::vector<vec<3>> offsets;

    g::game::sdf sdf;
    std::function<V(const g::game::sdf& sdf, const vec<3>& pos)> generator;
    float scale = 100;
    unsigned depth = 4;
    std::vector<terrain_block*> to_regenerate;
    g::proc::thread_pool<5> generator_pool;

    terrain_volume() = default;

    terrain_volume(const g::game::sdf& sdf, std::function<V(const g::game::sdf& sdf, const vec<3>& pos)> generator, const std::vector<vec<3>>& offset_config) :
        offsets(offset_config)
    {
        this->sdf = sdf;
        this->generator = generator;

        for (auto offset : offsets)
        {
            terrain_block block;
            block.mesh = g::gfx::mesh_factory{}.empty_mesh<V>();

            auto pipo = offset.template cast<int>();

            block.bounding_box[0] = (pipo * scale).template cast<float>();
            block.bounding_box[1] = ((pipo + 1) * scale).template cast<float>();

            block.index = (offset * scale).template cast<int>();

            time_t start = time(NULL);
            block.mesh.from_sdf_r(sdf, generator, block.bounding_box, depth);
            time_t end = time(NULL);

            blocks.push_back(block);
        }
    }

    void update(const g::game::camera& cam)
    {
        auto pos = cam.position;
        std::set<unsigned> unvisited;

        for (unsigned i = 0; i < offsets.size(); i++) { unvisited.insert(i); }

        auto pidx = ((pos / scale) - 0.5f).template cast<int>();

        for (auto& block : blocks)
        {
            if (block.regenerating) { continue; }

            bool needs_regen = true;

            for (auto oi : unvisited)
            {
                auto pipo = pidx + offsets[oi].template cast<int>();

                if (block.contains(pipo))
                {
                    unvisited.erase(oi);
                    needs_regen = false;
                    break;
                }
            }

            if (needs_regen) { to_regenerate.push_back(&block); }
        }

        
        assert(to_regenerate.size() == unvisited.size());

        // remaining 'unvisited' offset positions need to be regenerated
        if (to_regenerate.size() > 0)
        for (auto oi : unvisited)
        {
            auto offset = offsets[oi];
            auto block_ptr = to_regenerate.back();
            block_ptr->regenerating = true;
            to_regenerate.pop_back();

            generator_pool.run([this, oi, pidx, offset, block_ptr](){
                // auto ppo = (((pos / scale).floor() + 0.5f) + offsets[oi]) * scale;
                auto pipo = pidx + offsets[oi].template cast<int>();

                block_ptr->bounding_box[0] = (pipo * scale).template cast<float>();
                block_ptr->bounding_box[1] = ((pipo + 1) * scale).template cast<float>();
                block_ptr->index = pipo;

                block_ptr->mesh.from_sdf_r(sdf, generator, block_ptr->bounding_box, depth);
                block_ptr->regenerating = false;
            });
        }
    }

    void draw(g::game::camera& cam, g::gfx::shader& s, std::function<void(g::gfx::shader::usage&)> draw_config=nullptr)
    {
        auto model = mat4::I();

        for (auto& block : blocks)
        {
            auto chain = block.mesh.using_shader(s)
                ["u_model"].mat4(model)
                .set_camera(cam);

            if (draw_config) { draw_config(chain); }

            chain.template draw<GL_TRIANGLES>();

//            g::gfx::debug::print{&cam}.color({1, 1, 1, 1}).box(block.bounding_box);
        }
    }
};

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
    g::game::camera_perspective cam;
    // g::gfx::mesh<g::gfx::vertex::pos_uv_norm> terrain;
    std::vector<int8_t> v[3];

    terrain_volume<g::gfx::vertex::pos_uv_norm>* terrain;

    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

        // terrain = g::gfx::mesh_factory{}.empty_mesh<g::gfx::vertex::pos_uv_norm>();

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

        auto sdf = [&](const vec<3>& p) -> float {
            auto r = sqrtf(p.dot(p));
            auto base = r - 500.f;
            //d += g::gfx::perlin(p * 9, v) * 0.01;
            // d += g::gfx::perlin(p * 11, v) * 0.01;
            // d += g::gfx::perlin(p * 3, v) * 0.1;

            // if (base <= 0) return base;

            // auto d = p[1] - 200;
            // d += g::gfx::perlin(p*4.03, v[0]) * 0.125;
            // d += g::gfx::perlin(p*1.96, v[1]) * 0.25;

            auto d = base;
            d += g::gfx::perlin(p*0.065, v[0]) * 10;
            d += g::gfx::perlin(p*0.0334, v[1]) * 20;
            d += g::gfx::perlin(p*0.0123, v[2]) * 40;
            // d = std::max<float>(d, -1);

            return d;
        };

        auto generator = [](const g::game::sdf& sdf, const vec<3>& pos) -> g::gfx::vertex::pos_uv_norm
        {
            g::gfx::vertex::pos_uv_norm v;

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
        
            return v;
        };

        // std::vector<vec<3>> offsets = {{0, 0, 0}};
        std::vector<vec<3>> offsets;
        auto k = 2;
        for (float x = -k; x <= k; x++)
        for (float y = -k; y <= k; y++)
        for (float z = -k; z <= k; z++)
        {
            offsets.push_back({x, y, z});
        }

        terrain = new terrain_volume<g::gfx::vertex::pos_uv_norm>(sdf, generator, offsets);

        // vec<3> corners[2] = { {-10, -2, -10}, {10, 10, 10} };

        // time_t start = time(NULL);
        // terrain.from_sdf(sdf, generator, corners, 16);
        // time_t end = time(NULL);

        // std::cerr << "processing time: " << end - start << std::endl;

        cam.position = {0, 501, 0};
        //glDisable(GL_CULL_FACE);

        return true;
    }

    virtual void update(float dt)
    {
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto speed = 4.0f;
        if (glfwGetKey(g::gfx::GLFW_WIN, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) speed *= 4;
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

        auto model = mat4::I();

        terrain->update(cam);

        auto& wall = assets.tex("rock_wall.repeating.png");
        auto& ground = assets.tex("sand.repeating.png");
        terrain->draw(cam, assets.shader("planet.vs+planet_color.fs"), [&](g::gfx::shader::usage& usage) {
            usage["u_wall"].texture(wall)
                 ["u_ground"].texture(ground);
        });

        // terrain.using_shader(assets.shader("basic_gui.vs+debug_normal.fs"))
        //     ["u_model"].mat4(model)
        //     .set_camera(cam)
        //     .draw<GL_TRIANGLES>();
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
    core.start({ "046.heightmap", true, 512, 512 });
// #endif

    return 0;
}
