#pragma once

#include <limits>

#ifndef XMTYPE
#define XMTYPE float
#endif
#include <xmath.h>

namespace g {
namespace dyn {

using namespace xmath;

struct particle
{
	vec<3> position;
	vec<3> velocity;

	inline void dyn_step(float dt) { position += velocity * dt; }
};


struct rigid_body : public particle
{
private:
	mat<3, 3> _inertia_tensor_inv = {
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
	};
    mat<3, 3> _inertia_tensor = {
        { 1, 0, 0 },
        { 0, 1, 0 },
        { 0, 0, 1 },
    };

    vec<3> _net_f_local = {}; /**< Net force applied to the body wrt its CoM */
    vec<3> _net_t_local = {}; /**< Net torque applied to the body wrt its CoM */

public:
	float mass;
	vec<3> angular_momentum = {};
    vec<3> linear_momentum = {};
	quat<> orientation = {0, 0, 0, 1};

    void update_inertia_tensor()
    {
        mat<3, 3> L = {
            {mass, 0, 0},
            {0, mass, 0},
            {0, 0, mass},
        };

        _inertia_tensor = L;
        _inertia_tensor_inv = L;
        _inertia_tensor_inv.invert_inplace();
    }

    inline vec<3> acceleration() { return _net_f_local / mass; }

    inline vec<3> angular_velocity()
    {
        return _inertia_tensor_inv * angular_momentum;
    }

    inline vec<3> linear_velocity_at(const vec<3>& local_point)
    {
        return vec<3>::cross(to_global(angular_velocity()), to_global(local_point)) + velocity;
    }

	void update_inertia_tensor(const mat<3, 3>& L)
	{
        _inertia_tensor = L;
		_inertia_tensor_inv = L;
		_inertia_tensor_inv.invert_inplace();
	}

	void dyn_step(float dt)
	{
        auto apply_torque = _net_t_local.magnitude() >= 0.001;
        auto apply_force = _net_f_local.magnitude() >= 0.001;
        // if (apply_torque)
        {
            // apply angular momentum changes
            angular_momentum += (_net_t_local * dt) * apply_torque;
            _net_t_local = _net_t_local * (1 - apply_torque);
        }

        //if(apply_force)
        {
            // apply velocity changes
            linear_momentum += orientation.inverse().rotate(_net_f_local * dt) * apply_force;
            // velocity += (orientation.inverse().rotate(_net_f_local * dt) / mass) * apply_force;
            _net_f_local = _net_f_local * (1 - apply_force);
        }

        velocity = linear_momentum / mass;
		position += velocity * dt;
        auto w = angular_velocity();
        auto rad_sec = w.magnitude();
        auto is_spinning = rad_sec > 0;

        // if (is_spinning)
        {
            auto axis = (w / (rad_sec + 0.0000000001f)) * is_spinning;
            orientation *= quat<>::from_axis_angle(axis, rad_sec * dt * is_spinning);
        }
	}

    vec<3> to_local(const vec<3>& global)
    {
        return orientation.rotate(global);
    }

    vec<3> to_global(const vec<3>& local)
    {
        return orientation.inverse().rotate(local);
    }

    void dyn_apply_local_force(const vec<3>& point, const vec<3>& force)
    {
        auto f = force;
        auto& r = point;
        auto t = (vec<3>::cross(r, force));

        // _inertia_tensor_inv

        // f.unit().dot(r.)

        _net_t_local += t;
        _net_f_local += force;
    }

    void dyn_apply_global_force(const vec<3>& point, const vec<3>& force)
    {
        auto f = to_local(force);
        auto r = to_local(point - position);
        auto t = vec<3>::cross(r, f);

        _net_t_local += t;
        _net_f_local += f;
    }
};

namespace cd
{

struct intersect
{
    float time = std::numeric_limits<float>::quiet_NaN();
    vec<3> position = {}, normal = {};

    intersect() = default;

    intersect(float t, const vec<3>& p, const vec<3>& n) : time(t), position(p), normal(n) {}
};

struct ray
{
    vec<3> position, direction;

    inline vec<3> point_at(float t) const { return position + direction * t; }
};

struct box
{
    vec<3> position;
    vec<3> half_x;
    vec<3> half_y;
    vec<3> half_z;
};

static intersect ray_plane(const ray& r, const vec<3>& plane_o, const vec<3>& plane_n)
{
    auto t = xmath::intersect::ray_plane(r.position, r.direction, plane_o, plane_n);

    return {
        t,
        r.point_at(t),
        plane_n
    };
}

static intersect ray_box(const ray& r, const box& b)
{
    vec<3> half_lengths[] = {b.half_x, b.half_y, b.half_z};
    auto t = xmath::intersect::ray_box(r.position, r.direction, b.position, half_lengths);
    auto face_normal = vec<3>{}; // TODO: compute normal here

    return {
        t,
        r.point_at(t),
        face_normal
    };
}

} // end namespace cd
} // end namespace dyn
} // end namespace g
