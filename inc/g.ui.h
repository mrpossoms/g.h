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
    none,
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
            ui::event event = ui::event::none;
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

        itr(layer& l, const ui::pointer* p, event e);
        
        bool operator!=(const itr& it);

        void operator++();

        context& operator*();
    };

    struct description
    {
        std::string program_collection;
        std::string font;
    
        std::function<void (shader::usage&)> draw;
    };

protected:
    g::asset::store* assets = nullptr;
    ui::pointer* cur_pointer = nullptr;
    mat<4, 4> transform = mat<4, 4>::I();
    description desc;

    mesh<vertex::pos_uv_norm>& plane();

public:
    layer(g::asset::store* store, const std::string& program_collection);

    layer(g::asset::store* store, const std::string& program_collection, const mat<4, 4>& trans);

    layer(const g::ui::layer& parent, const mat<4, 4>& trans);

    layer::itr begin() { return { *this, cur_pointer, event::none, }; }
    layer::itr end() { return { *this, cur_pointer, event::last, }; }

    layer& set_font(const std::string& font_asset);

    layer& set_pointer(ui::pointer* pointer);

    layer& set_shaders(const std::string& program_collection);

    layer child(const vec<2>& dimensions, const vec<3>& position);

    // TODO: this should return a usage instead
    shader::usage text(const std::string& str, g::game::camera& cam);

    shader::usage using_shader();

    shader::usage using_shader(const std::string& program_collection);

    float intersects(const vec<3>& ray_o, const vec<3>& ray_d) const;

    bool hover(const pointer& p) const;

    bool select(const pointer& p, unsigned pick_value=1);
};


pointer pointer_from_mouse(g::game::camera* cam);


xmath::vec<3> origin_from_mouse(g::game::camera* cam);


xmath::vec<3> cast_from_mouse(g::game::camera* cam);


} // end namespace ui
} // end namespace g
