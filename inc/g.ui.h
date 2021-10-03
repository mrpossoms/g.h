#pragma once

#include "g.gfx.h"
#include "g.assets.h"
#include "g.utils.h"

using namespace xmath;
using namespace g::gfx;

namespace g
{
namespace ui
{

enum class event
{
    hover,
    select,
    draw,
    last,
};

struct pointer
{
    vec<3> position;
    vec<3> direction;
    uint32_t pick; //< Bitfield which corresponds to combinations of buttons or actions. 0 for none
    float analog[8]; //< Analog control inputs, intended for things like joysticks and track pads
};

struct layer
{
    struct itr
    {
        struct context
        {
            ui::event event = ui::event::hover;
            shader::usage draw;
            bool triggered = false;

            context() = default;

            context(ui::event e)
            {
                event = e;
            }
        };

        unsigned i;
        layer& layer_ref;
        const ui::pointer* pointer_ptr;
        context ctx;

        itr(layer& l, const ui::pointer* p, event e) : layer_ref(l), pointer_ptr(p)
        {
            i = static_cast<unsigned>(e);
        }

        bool operator!=(const itr& it) { return it.i != i; }

        void operator++()
        { 
            g::utils::inc_at_end<unsigned> inc(i);
            
            while(i <= static_cast<unsigned>(event::draw))
            {
                ctx = {static_cast<ui::event>(i)};
                switch(ctx.event)
                {
                    case event::hover:
                        if (nullptr != pointer_ptr)
                        {
                            if (ctx.triggered = layer_ref.hover(*pointer_ptr)) return;
                        }
                        break;
                    case event::select:
                        if (nullptr != pointer_ptr)
                        {
                            if (ctx.triggered = layer_ref.select(*pointer_ptr)) return;
                        }
                        break;
                    case event::draw:
                        ctx.draw = layer_ref.using_shader();
                        return;
                }

                i++;
            }
        }

        context& operator*()
        {
            return ctx;
        }
    };

protected:
    g::asset::store* assets = nullptr;
    ui::pointer* cur_pointer = nullptr;
    mat<4, 4> transform = mat<4, 4>::I();
    std::string program_collection;
    // shader::usage shader_usage;
    std::string font;

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
        assets = store;
        this->program_collection = program_collection;
    }

    layer(g::asset::store* store, const std::string& program_collection, const mat<4, 4>& trans)
    {
        assets = store;
        this->program_collection = program_collection;
        transform = trans;
    }

    layer(const g::ui::layer& parent, const mat<4, 4>& trans)
    {
        assets = parent.assets;
        transform = parent.transform;
        program_collection = parent.program_collection;
        font = parent.font;
        transform *= trans;
    }

    layer::itr begin() { return { *this, cur_pointer, event::hover, }; }
    layer::itr end() { return { *this, cur_pointer, event::last, }; }

    layer& set_font(const std::string& font_asset)
    {
        font = font_asset;
        return *this;
    }

    layer& set_pointer(ui::pointer* pointer)
    {
        cur_pointer = pointer;
        return *this;
    }

    layer& set_shaders(const std::string& program_collection)
    {
        this->program_collection = program_collection;
        return *this;
    }

    layer child(const vec<2>& dimensions, const vec<3>& position)
    {
        auto trans = mat<4, 4>::translation(position) * mat<4, 4>::scale({ dimensions[0], dimensions[1], 0.1f });

        layer c(*this, trans);

        c.transform = mat<4, 4>::scale({ dimensions[0], dimensions[1], 0.1f }) * transform * mat<4, 4>::translation(position);

        return c;
    }

    // TODO: this should return a usage instead
    void text(const std::string& str, g::game::camera& cam)
    {
        auto text = g::gfx::primative::text{assets->font(font)};

        vec<2> dims, offset;
        text.measure(str, dims, offset);

        auto text_aspect = dims[1] / dims[0];
        auto scl = vec<2>{ 2 / dims[0], text_aspect * 2 / dims[1] };
        // offset *= scl;
        offset[1] = 0;
        auto trans = (dims * -0.5);// +offset;// - offset * 0.5;
        auto model = mat<4, 4>::translation({ trans[0], trans[1], 0}) * mat<4, 4>::scale({ scl[0], scl[1], 0 });

        text.draw(assets->shader(program_collection), str, cam, model * transform);

        assets->font(font).face.unbind();
    }

    shader::usage using_shader()
    {
        const auto I = mat<4, 4>::I();

        return plane().using_shader(assets->shader(program_collection))
        ["u_model"].mat4(transform)
        ["u_view"].mat4(I)
        ["u_proj"].mat4(I);
    }

    shader::usage using_shader(const std::string& program_collection)
    {
        this->program_collection = program_collection;
        return using_shader();
    }

    float intersects(const vec<3>& ray_o, const vec<3>& ray_d) const
    {
        auto t = vec<3>{ transform[3][0], transform[3][1], transform[3][1] };

        vec<3> half_lengths[3] = {
            { transform[0][0], transform[0][1], transform[0][2] },
            { transform[1][0], transform[1][1], transform[1][2] },
            { transform[2][0], transform[2][1], transform[2][2] },
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
    pointer pointer_out = {};
    double xpos, ypos;
    glfwGetCursorPos(GLFW_WIN, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(GLFW_WIN, &width, &height);

    auto eps = 0.000001f;
    auto proj = cam->projection();
    // auto is_proj_ortho = fabs(proj[0].dot(proj[1])) < eps &&
    //                      fabs(proj[1].dot(proj[2])) < eps &&
    //                      fabs(proj[2].dot(proj[0])) < eps;

    const int buttons[] = {
        GLFW_MOUSE_BUTTON_1,
        GLFW_MOUSE_BUTTON_2,
        GLFW_MOUSE_BUTTON_3,
        GLFW_MOUSE_BUTTON_4,
        GLFW_MOUSE_BUTTON_5,
        GLFW_MOUSE_BUTTON_6,
        GLFW_MOUSE_BUTTON_7,
        GLFW_MOUSE_BUTTON_8,
    };

    // check each button for presses
    for (unsigned bi = 0; bi < sizeof(buttons)/sizeof(buttons[0]); bi++)
    {
        pointer_out.pick |= glfwGetMouseButton(GLFW_WIN, buttons[bi]) << bi;
    }


    auto view = cam->view();
    auto view_rot = cam->orientation.to_matrix();

    // TODO: figure out where to position ray origins for orthographic
    // projections
    // if (is_proj_ortho)
    // {
    // // ...
    // }
    pointer_out.position = cam->position;

    pointer_out.analog[0] = xpos / width; // mouse x-axis
    pointer_out.analog[1] = ypos / height; // mouse y-axis

    {
        // unproject the point of the mouse on screen into world space
        auto ray_d_unproj = proj.transpose().invert() * xmath::vec<4>{
            (2.f * pointer_out.analog[0] - 1.f),
            -(2.f * pointer_out.analog[1] - 1.f),
            1.f,
            1.f
        };

        // rotate the unprojected point
        auto ray_d = view_rot * ray_d_unproj;
        pointer_out.direction = ray_d.slice<3>() / ray_d[3];
    }

    return pointer_out;
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
