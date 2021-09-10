#pragma once

#include "g.gfx.h"
#include "g.assets.h"

using namespace xmath;
using namespace g::gfx;

namespace g::ui
{

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

    layer(const g::ui::layer& parent, const mat<4, 4>& trans)
    {
        ctx = parent.ctx;
        ctx.transform *= trans;
    }

    layer child(const vec<2>& dimensions, const vec<3>& position)
    {
        auto transform = mat<4, 4>::translation(position) * mat<4, 4>::scale({dimensions[0], dimensions[1], 1});

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



    bool mouse_hover() { return false; }

    bool mouse_click() { return false; }
};

} // end namespace g::ui
