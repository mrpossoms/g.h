#pragma once

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

public:
	float mass;
	vec<3> angular_momentum = {};
	quat<> orientation = {0, 0, 0, 1};

	void update_inertia_tensor(const mat<3, 3>& L)
	{
		_inertia_tensor_inv = L;
		_inertia_tensor_inv.invert_inplace();
	}

	void dyn_step(float dt)
	{
		position += velocity * dt;
        auto w = _inertia_tensor_inv * angular_momentum;
        auto ang_mag = angular_momentum.magnitude();
        if (ang_mag > 0)
        {
            auto axis = angular_momentum / ang_mag;
            orientation *= quat<>::from_axis_angle(axis, w.magnitude() * dt);
        }
	}
};

} // end namespace dyn
} // end namespace g