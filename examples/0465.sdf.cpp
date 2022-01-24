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
    float scale = 10;
    unsigned depth = 2;

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

            auto pipo = offset.cast<int>();

            block.bounding_box[0] = (pipo * scale).cast<float>();
            block.bounding_box[1] = ((pipo + 1) * scale).cast<float>();

            block.index = (offset * scale).cast<int>();

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
        std::vector<terrain_block*> to_regenerate;

        for (unsigned i = 0; i < offsets.size(); i++) { unvisited.insert(i); }

        auto pidx = ((pos / scale) - 0.5f).cast<int>();

        for (auto& block : blocks)
        {
            bool needs_regen = true;

            for (auto oi : unvisited)
            {
                auto pipo = pidx + offsets[oi].cast<int>();

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

            // auto ppo = (((pos / scale).floor() + 0.5f) + offsets[oi]) * scale;
            auto pipo = pidx + offsets[oi].cast<int>();

            block_ptr->bounding_box[0] = (pipo * scale).cast<float>();//ppo - vec<3>{0.5f, 0.5f, 0.5f} * scale;
            block_ptr->bounding_box[1] = ((pipo + 1) * scale).cast<float>();//ppo + vec<3>{0.5f, 0.5f, 0.5f} * scale;
            block_ptr->index = pipo;

            std::cerr << "generating " << block_ptr->bounding_box[0].to_string() << " - " << block_ptr->bounding_box[1].to_string() << std::endl;

            time_t start = time(NULL);
            block_ptr->mesh.from_sdf_r(sdf, generator, block_ptr->bounding_box, depth);
            time_t end = time(NULL);

            std::cerr << "processing time: " << end - start << " verts: " << block_ptr->mesh.vertex_count << std::endl;

            to_regenerate.pop_back();
        }
    }

    void draw(g::game::camera& cam, g::gfx::shader& s)
    {
        auto model = mat4::I();

        for (auto& block : blocks)
        {
            block.mesh.using_shader(s)
                ["u_model"].mat4(model)
                .set_camera(cam)
                .template draw<GL_TRIANGLES>();            
        }
    }
};

struct my_core : public g::core
{
    g::gfx::shader basic_shader;
    g::asset::store assets;
    g::game::camera_perspective cam;
    // g::gfx::mesh<g::gfx::vertex::pos_uv_norm> terrain;
    std::vector<int8_t> v[3];

    terrain_volume<g::gfx::vertex::pos_uv_norm> terrain;

    virtual bool initialize()
    {
        std::cout << "initialize your game state here.\n";

        // terrain = g::gfx::mesh_factory{}.empty_mesh<g::gfx::vertex::pos_uv_norm>();

        srand(time(NULL));

        for (unsigned i = 1024; i--;)
        {
            v[0].push_back(rand() % 255 - 128);
            v[1].push_back(rand() % 255 - 128);
            v[2].push_back(rand() % 255 - 128);
        }

        auto sdf = [&](const vec<3>& p) -> float {
            auto d = sqrtf(p.dot(p)) - 200.f;
            //d += g::gfx::perlin(p * 9, v) * 0.01;
            // d += g::gfx::perlin(p * 11, v) * 0.01;
            // d += g::gfx::perlin(p * 3, v) * 0.1;


            // auto d = p[1] - 200;
            d += g::gfx::perlin(p*4.03, v[0]) * 0.125;
            d += g::gfx::perlin(p*1.96, v[1]) * 0.25;
            d += g::gfx::perlin(p*0.1, v[2]) * 9;

            // d = std::max<float>(d, -1);

            return d;
        };

        auto test_sdf = [](const vec<3>& p)
        {
            return p[1] + cos(p[0]) * 0.1;
        };

        auto generator = [](const g::game::sdf& sdf, const vec<3>& pos) -> g::gfx::vertex::pos_uv_norm
        {
            g::gfx::vertex::pos_uv_norm v;

            v.position = pos;

            const float s = 0.1;
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
        auto k = 4;
        for (float x = -k; x <= k; x++)
        for (float y = -k; y <= k; y++)
        for (float z = -k; z <= k; z++)
        {
            offsets.push_back({x, y, z});
        }

        terrain = terrain_volume<g::gfx::vertex::pos_uv_norm>(sdf, generator, offsets);

        // vec<3> corners[2] = { {-10, -2, -10}, {10, 10, 10} };

        // time_t start = time(NULL);
        // terrain.from_sdf(sdf, generator, corners, 16);
        // time_t end = time(NULL);

        // std::cerr << "processing time: " << end - start << std::endl;

        cam.position = {0, 201, 0};
        //glDisable(GL_CULL_FACE);

        return true;
    }

    virtual void update(float dt)
    {
        glClearColor(0, 0, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

        auto model = mat4::I();

        terrain.update(cam);

        terrain.draw(cam, assets.shader("basic_gui.vs+debug_normal.fs"));

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
