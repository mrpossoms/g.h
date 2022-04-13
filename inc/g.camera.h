#pragma once
#include "g.game.h"
#include "g.dyn.h"

namespace g {
namespace game {
 

struct camera : public pose
{
	quat<float>& d_pitch(float delta)
	{
		auto dq = quat<float>::from_axis_angle({1, 0, 0}, delta);
		return orientation *= dq;
	}

	quat<float>& d_yaw(float delta)
	{
		auto dq = quat<float>::from_axis_angle({0, 1, 0}, delta);
		return orientation *= dq;
	}

	quat<float>& d_roll(float delta)
	{
		auto dq = quat<float>::from_axis_angle({0, 0, 1}, delta);
		return orientation *= dq;
	}

	virtual vec<3> forward() const { return orientation.rotate({0, 0, -1}); }

	virtual vec<3> left() const { return orientation.rotate({-1, 0, 0}); }

	virtual vec<3> up() const { return orientation.rotate({0, 1, 0}); }

	void look_at(const vec<3>& subject, const vec<3>& up={0, 1, 0})
	{
		auto forward = (position - subject).unit();
		auto d = forward.dot({ 0, 0, 1 });

		if (fabsf(d + 1.f) < 0.000001f)
		{
			orientation = quat<>(0, 1, 0, M_PI);
		}
		else if (fabsf(d - 1.f) < 0.000001f)
		{
			orientation = quat<>{};
		}
		else
		{
			auto angle = acosf(d);
			auto axis = vec<3>::cross({ 0, 0, 1 }, forward).unit();

			orientation = quat<>::from_axis_angle(axis, angle);
		}

		//return _view = mat<4, 4>::look_at(position, (position - subject).unit(), up);
	}

	void look_at(const vec<3>& pos, const vec<3>& forward, const vec<3>& up)
	{
		_view = mat<4, 4>::look_at((position = pos), forward, up);
	}

	virtual mat<4, 4> view() const
	{
		if (_view[3][3] != 0)
		{ 
			return _view; 
		}
		return mat<4, 4>::translation(position * -1) * orientation.to_matrix();
	}

	virtual mat<4, 4> projection() const = 0;

	virtual void aspect_ratio(float aspect) {};

private:
	mat<4, 4> _view = {};
};

struct camera_perspective : public camera
{
	float field_of_view = M_PI / 2;
	float near = 0.01f, far = 1000.f;
	float aspect = 1;

	virtual mat<4, 4> projection() const
	{
		return mat<4, 4>::perspective(near, far, field_of_view, aspect);
	}

	virtual void aspect_ratio(float aspect) { this->aspect = aspect; }
};

struct camera_orthographic : public camera
{
	float near = 0.1f, far = 1000.f;
	float width = 10, height = 10;

	virtual mat<4, 4> projection() const
	{
		return mat<4, 4>::orthographic(near, far, width/2, -width/2, height/2, -height/2);
	}
};


using namespace g::dyn::cd;

struct fps_camera : public camera_perspective, updateable, ray_collider
{
	vec<3> velocity = {};
	vec<3> foot_offset = {0, -1, 0};
	vec<3> gravity = { 0, -9.8, 0 };
	float drag = 0.3;

	bool touching_surface = false;

	std::function<void(fps_camera& cam, float dt)> on_input;

	quat<> q, r;
	quat<> yaw_q;

	float speed = 1;
	float yaw = 0;
	float pitch = 0;
	const float max_pitch = (M_PI / 2.f) - 0.1f;
	const float min_pitch = -(M_PI / 2.f) + 0.1f;

    std::vector<ray>& rays() override
    {
        ray_list.clear();
        ray_list.push_back({feet(), dir});
        ray_list.push_back({position + body_forward() * 2, dir});
        //rays.push_back({position - body_forward() * 2, dir});
        //rays.push_back({position + body_left() * 2, dir});
        //rays.push_back({position - body_left() * 2, dir});

        return ray_list;
    }

	vec<3> forward() const override 
	{
		return (r * q).rotate({ 0, 0, -1 });
	}

	vec<3> left() const override
	{
		return (r * q).rotate({ 1, 0, 0 });
	}

	vec<3> up() const override
	{
		return q.rotate({ 0, 1, 0 });
	}

	vec<3> body_forward() const
	{
		return (yaw_q * q).rotate({ 0, 0, -1 });
	}

	vec<3> body_left() const
	{
		return (yaw_q * q).rotate({ 1, 0, 0 });
	}

	vec<3> body_up() const
	{
		return q.rotate({ 0, 1, 0 });
	}

	vec<3> feet() const
	{
		return position + q.rotate(foot_offset);
	}

	void pre_update(float dt, float time) override
	{
		if (on_input) on_input(*this, dt);

		auto up = -gravity.unit();
		auto d = up.dot({ 0, 1, 0 });
		auto a = acos(d);
		auto axis = vec<3>::cross(up, { 0, 1, 0 });
		if (axis.magnitude() < 0.0001) { axis = { 1, 0, 0 }; }
		axis = axis.unit();
		q = quat<>::from_axis_angle(axis, a).inverse();

		pitch = std::min<float>(max_pitch, std::max<float>(min_pitch, pitch));

		yaw_q = quat<>::from_axis_angle(body_up(), yaw);
		r = quat<>::from_axis_angle(body_left(), pitch) * yaw_q;
		orientation = r * q;		


		velocity += -velocity * drag * dt;
		velocity += gravity * dt;

		dir = velocity * dt;

	}

	void update(float dt, float t) override
	{
		//velocity += gravity * dt;

		position += velocity * dt;
	}
private:
	vec<3> dir; //< used for ray generation in collision detection
};

} // namespace game

} // namespace g