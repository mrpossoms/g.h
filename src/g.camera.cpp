#include "g.camera.h"

using namespace g::game;

quat<float>& camera::d_pitch(float delta)
{
	auto dq = quat<float>::from_axis_angle({1, 0, 0}, delta);
	return orientation *= dq;
}

quat<float>& camera::d_yaw(float delta)
{
	auto dq = quat<float>::from_axis_angle({0, 1, 0}, -delta);
	return orientation *= dq;
}

quat<float>& camera::d_roll(float delta)
{
	auto dq = quat<float>::from_axis_angle({0, 0, 1}, -delta);
	return orientation *= dq;
}

vec<3> camera::forward() const { return orientation.rotate({0, 0, -1}); }

vec<3> camera::left() const { return orientation.rotate({-1, 0, 0}); }

vec<3> camera::up() const { return orientation.rotate({0, 1, 0}); }

// void camera::look_at(const vec<3>& subject, vec<3> up)
// {
// 	auto forward = (position - subject).unit();
// 	auto d = forward.dot({ 0, 0, 1 });

// 	if (fabsf(d + 1.f) < 0.000001f)
// 	{
// 		orientation = quat<>(0, 1, 0, M_PI);
// 	}
// 	else if (fabsf(d - 1.f) < 0.000001f)
// 	{
// 		orientation = quat<>{};
// 	}
// 	else
// 	{
// 		auto angle = acosf(d);
// 		auto axis = vec<3>::cross({ 0, 0, 1 }, forward).unit();

// 		orientation = quat<>::from_axis_angle(axis, angle);
// 	}

// 	auto right = vec<3>::cross(forward, up).unit();
// 	up = vec<3>::cross(right, forward).unit();

// 	auto view_up = orientation.rotate({ 0, 1, 0 }).project_onto_plane(forward).unit();
// 	auto target_up = up.project_onto_plane(forward).unit();
// 	auto angle = acosf(view_up.dot(target_up));

// 	orientation = quat<>::from_axis_angle(forward, -angle) * orientation;

// 	//return _view = mat<4, 4>::look_at(position, (position - subject).unit(), up);
// }

void camera::look_at(const vec<3>& subject, vec<3> up)
{
	vec<3> forward_basis = {0, 0, 1};
	auto target_forward = (position - subject).unit();
	auto actual_forward = orientation.rotate(forward_basis);

	auto d_angle = actual_forward.angle_to(target_forward);

	if (d_angle > 0.0001f)
	{
		auto axis = vec<3>::cross(actual_forward, target_forward).unit();
		auto local_right = vec<3>::cross(actual_forward, axis).unit();
		auto sign = local_right.dot(target_forward) < 0 ? 1 : -1;

		orientation = quat<>::from_axis_angle(axis, d_angle * sign) * orientation;
		actual_forward = orientation.rotate(forward_basis);
	}
	
	vec<3> up_basis = {0, 1, 0};
	auto target_up = up.project_onto_plane(actual_forward).unit();
	auto actual_up = orientation.rotate(up_basis).unit().project_onto_plane(actual_forward).unit();

	auto d_angle_up = actual_up.angle_to(target_up);

	if (d_angle_up > 0.0001f)
	{
		auto axis = actual_forward;
		auto local_right = vec<3>::cross(actual_forward, target_up).unit();
		auto sign = local_right.dot(actual_up) < 0 ? 1 : -1;

		orientation = quat<>::from_axis_angle(actual_forward, d_angle_up * sign) * orientation;
		// actual_up = orientation.rotate(up_basis).project_onto_plane(actual_forward).unit();
	}


	// auto right = vec<3>::cross(actual_forward, up).unit();
	// up = vec<3>::cross(right, actual_forward).unit();

	// auto view_up = orientation.rotate({ 0, 1, 0 }).project_onto_plane(target_forward).unit();
	// auto target_up = up.project_onto_plane(target_forward).unit();
	// auto angle = view_up.angle_to(target_up);

	// orientation = quat<>::from_axis_angle(target_forward, angle) * orientation;
}

void camera::look_at(const vec<3>& pos, const vec<3>& forward, const vec<3>& up)
{
	_view = mat<4, 4>::look_at((position = pos), forward, up).transpose();
}

mat<4, 4> camera::view() const
{
	if (_view[3][3] != 0)
	{ 
		return _view; 
	}
	return orientation.inverse().to_matrix() * mat<4, 4>::translation(position * -1);
}