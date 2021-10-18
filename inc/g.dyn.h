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

    vec<3> _net_f = {};
    vec<3> _net_d_l = {};

public:
	float mass;
	vec<3> angular_momentum = {};
	quat<> orientation = {0, 0, 0, 1};

	void update_inertia_tensor(const mat<3, 3>& L)
	{
        _inertia_tensor = L;
		_inertia_tensor_inv = L;
		_inertia_tensor_inv.invert_inplace();
	}

	void dyn_step(float dt)
	{
        if (_net_d_l.magnitude() >= 0.1 || _net_f.magnitude() >= 0.1)
        { // apply net angular momentum and velocity changes
            auto w = _inertia_tensor * _net_d_l;
            angular_momentum += orientation.inverse().rotate(w);
            velocity += orientation.inverse().rotate(_net_f * dt) / mass;
        }

		position += velocity * dt;
        auto w = _inertia_tensor_inv * angular_momentum;
        auto ang_mag = angular_momentum.magnitude();

        if (ang_mag > 0)
        {
            auto axis = angular_momentum / ang_mag;
            orientation *= quat<>::from_axis_angle(axis, w.magnitude() * dt);
        }
	}

    void dyn_apply_force(const vec<3>& point, const vec<3>& force)
    {
        auto& r = point;
        auto d_l = (vec<3>::cross(r, force));

        _net_d_l += d_l;
        _net_f += force;
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

static intersect ray_plane(const ray& r, const vec<3>& plane_o, const vec<3>& plane_n)
{
    auto t = xmath::intersect::ray_plane(r.position, r.direction, plane_o, plane_n);

    return {
        t,
        r.point_at(t),
        plane_n
    };
}

} // end namespace cd
} // end namespace dyn
} // end namespace g
