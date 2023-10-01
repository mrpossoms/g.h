#pragma once
#include "g.game.h"
#include "g.dyn.h"

namespace g {
namespace game {
 

struct camera : public pose
{
	quat<float>& d_pitch(float delta);

	quat<float>& d_yaw(float delta);

	quat<float>& d_roll(float delta);

	virtual vec<3> forward() const;

	virtual vec<3> left() const;

	virtual vec<3> up() const;

	void look_at(const vec<3>& subject, vec<3> up={0, 1, 0});

	void look_at(const vec<3>& pos, const vec<3>& forward, const vec<3>& up);

	virtual mat<4, 4> view() const;

	virtual mat<4, 4> projection() const = 0;

	virtual void aspect_ratio(float aspect) {};

private:
	mat<4, 4> _view = {};
};

struct camera_perspective : public camera
{
	float near = 0.01f, far = 1000.f;
	float fov_horizontal = M_PI / 2;
	float fov_vertical = M_PI / 2;

	virtual mat<4, 4> projection() const
	{
		return mat<4, 4>::perspective(near, far, fov_horizontal, fov_vertical);
	}

	virtual void aspect_ratio(float aspect) 
	{ 
		fov_vertical = fov_horizontal / aspect;
	}

	virtual void field_of_view(float fov)
	{
		fov_horizontal = fov;
		fov_vertical = fov;
	}
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
		return (yaw_q * q).rotate({ -1, 0, 0 });
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

		auto up = vec<3>{0, 1, 0};
		auto grav_mag = gravity.magnitude();

		if (grav_mag > std::numeric_limits<float>::epsilon())
		{
			up = -gravity / grav_mag;
		}

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