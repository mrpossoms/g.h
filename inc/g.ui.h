#pragma once

#include "g.gfx.h"
#include "g.assets.h"

using namespace xmath;
using namespace g::gfx;

namespace g
{
namespace ui
{

struct pointer
{
    vec<3> position;
    vec<3> direction;
    uint32_t pick; //< Bitfield which corresponds to combinations of buttons or actions. 0 for none
};

struct layer
{
    struct context
    {
        g::asset::store* assets;
        mat<4, 4> transform = mat<4, 4>::I();
        std::string program_collection;
        // shader::usage shader_usage;
    };

protected:
    context ctx;

    mesh<vertex::pos_uv_norm>& plane()
    {
        static mesh<vertex::pos_uv_norm> mesh;
        if (!mesh.is_initialized())
        {
            mesh = mesh_factory::plane();
        }

        return mesh;
    }

public:
    layer(g::asset::store* store, const std::string& program_collection)
    {
        ctx.assets = store;
        ctx.program_collection = program_collection;
    }

    layer(g::asset::store* store, const std::string& program_collection, const mat<4, 4>& trans)
    {
        ctx.assets = store;
        ctx.program_collection = program_collection;
        ctx.transform = trans;
    }

    layer(const g::ui::layer& parent, const mat<4, 4>& trans)
    {
        ctx = parent.ctx;
        ctx.transform *= trans;
    }

    layer child(const vec<2>& dimensions, const vec<3>& position)
    {
        auto transform = mat<4, 4>::scale({dimensions[0], dimensions[1], 0.1f}) * mat<4, 4>::translation(position);

        return { *this, transform };
    }

    shader::usage shader()
    {
        const auto I = mat<4, 4>::I();

        return plane().using_shader(ctx.assets->shader(ctx.program_collection))
        ["u_model"].mat4(ctx.transform)
        ["u_view"].mat4(I)
        ["u_proj"].mat4(I);
    }

    shader::usage shader(const std::string& program_collection)
    {
        ctx.program_collection = program_collection;
        return shader();
    }

    float intersects(const vec<3>& ray_o, const vec<3>& ray_d) const
    {
        auto t = vec<3>{ ctx.transform[3][0], ctx.transform[3][1], ctx.transform[3][1] };

        vec<3> half_lengths[3] = {
            { ctx.transform[0][0], ctx.transform[0][1], ctx.transform[0][2] },
            { ctx.transform[1][0], ctx.transform[1][1], ctx.transform[1][2] },
            { ctx.transform[2][0], ctx.transform[2][1], ctx.transform[2][2] },
        };

        return intersect::ray_box(ray_o, ray_d, t, half_lengths);
    }

    bool hover(const pointer& p) const
    {
        return !isnan(intersects(p.position, p.direction));
    }

    bool select(const pointer& p, unsigned pick_value=1)
    {
        return hover(p) && p.pick == pick_value;
    }
};


static pointer pointer_from_mouse(g::game::camera* cam)
{
    double xpos, ypos;
    glfwGetCursorPos(GLFW_WIN, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(GLFW_WIN, &width, &height);

    auto eps = 0.000001f;
    auto proj = cam->projection();
    auto is_proj_ortho = fabs(proj[0].dot(proj[1])) < eps &&
                         fabs(proj[1].dot(proj[2])) < eps &&
                         fabs(proj[2].dot(proj[0])) < eps;

    vec<3> pos, dir;
    uint32_t pick;

    if (is_proj_ortho)
    {
        auto ray_pos = cam->view() * xmath::vec<4>{
            2.f * (float)(xpos / width) - 1.f,
            2.f * (float)(ypos / height) - 1.f,
            0.f,
            1.0f
        };
        pos = ray_pos.slice<3>();
    }

    {
        auto ray_d = cam->projection().invert() * xmath::vec<4>{
            2.f * (float)(xpos / width) - 1.f,
            2.f * (float)(ypos / height) - 1.f, 1.f, 1.f
        };
        dir = ray_d.slice<3>();
    }

    return { pos, dir, pick };
}


static xmath::vec<3> origin_from_mouse(g::game::camera* cam)
{
    double xpos, ypos;
    glfwGetCursorPos(GLFW_WIN, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(GLFW_WIN, &width, &height);

    return {
        2.f * (float)(xpos / width) - 1.f,
        2.f * (float)(ypos / height) - 1.f,
        0.f
    };
}


static xmath::vec<3> cast_from_mouse(g::game::camera* cam)
{
    double xpos, ypos;
    glfwGetCursorPos(GLFW_WIN, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(GLFW_WIN, &width, &height);

    auto ray_d = cam->projection().invert() * xmath::vec<4>{
        2.f * (float)(xpos / width) - 1.f,
        2.f * (float)(ypos / height) - 1.f, 1.f, 1.f
    };

    return ray_d.slice<3>();
}


} // end namespace ui
} // end namespace g
