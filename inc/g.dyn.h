#pragma once

#include <limits>

#ifndef XMTYPE
#define XMTYPE float
#endif
#include <xmath.h>
#include "g.game.h"

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

namespace cd //< Collision detection
{

struct box
{
    vec<3> position;
    vec<3> half_x;
    vec<3> half_y;
    vec<3> half_z;
};


struct intersection
{
    float time = std::numeric_limits<float>::quiet_NaN();
    vec<3> origin = {}, direction = {}, point = {}, normal = {};

    intersection() = default;

    intersection(float t, const vec<3>& o, const vec<3>& d, const vec<3>& p, const vec<3>& n) : time(t), origin(o), direction(d), point(p), normal(n) {}

    operator bool() { return !std::isnan(time); }
};

struct ray
{
    vec<3> position, direction;

    inline vec<3> point_at(float t) const { return position + direction * t; }

    intersection intersect_plane(const ray& r, const vec<3>& plane_o, const vec<3>& plane_n)
    {
        auto t = xmath::intersect::ray_plane(position, direction, plane_o, plane_n);

        return {
            t,
            position,
            direction,
            point_at(t),
            plane_n
        };
    }

    intersection intersect_box(const box& b)
    {
        vec<3> half_lengths[] = { b.half_x, b.half_y, b.half_z };
        auto t = xmath::intersect::ray_box(position, direction, b.position, half_lengths);
        auto face_normal = vec<3>{}; // TODO: compute normal here

        return {
            t,
            position,
            direction,
            point_at(t),
            face_normal
        };
    }
};

struct collider
{
    /**
     * @brief      Returns an intersection with this collider if one can be generated.
     *
     * @param[in]  r     Ray to test intersection against.
     *
     * @return     Intersection result from test.
     */
    virtual intersection ray_intersects(const ray& r) const = 0;
    
    /**
     * @brief      Indicates whether or not this collider can generate rays
     *
     * @return     True if rays are generated, false otherwise.
     */
    virtual bool generates_rays() = 0;

    /**
     * @brief      Returns a vector of rays cast from this collider.
     *
     * @return     Vector of arrays. This vector should be retained by the collider to avoid
     *             redundant allocations.
     */
    virtual std::vector<ray>& rays() = 0;

    /**
     * @brief      Given this collider, and another return a vector of all points of
     *             intersection between the two colliders
     *
     * @param[in]  other  The other collider to test collision with
     *
     * @return     { description_of_the_return_value }
     */
    virtual const std::vector<intersection>& intersections(collider& other, float max_t=std::numeric_limits<float>::infinity()) = 0;
};

struct ray_collider : public collider
{
    intersection ray_intersects(const ray& r) const override { return {}; }

    bool generates_rays() override { return true; }

    std::vector<ray>& rays() override { return ray_list; }

    const std::vector<intersection>& intersections(collider& other, float max_t = std::numeric_limits<float>::infinity()) override
    {
        intersection_list.clear();

        for (const auto& r : ray_list)
        {
            auto mag = r.direction.magnitude();

            if (mag == 0) { return intersection_list; }

            auto i = other.ray_intersects(r);
            if (i && i.time < max_t) { intersection_list.push_back(i); }
        }

        return intersection_list;
    }    

protected:
    std::vector<intersection> intersection_list;
    std::vector<ray> ray_list;
};

struct sdf_collider : public collider
{
    sdf_collider(const g::game::sdf& s) : sdf(s) {}

    intersection ray_intersects(const ray& r) const override
    {
        // constexpr auto stop_threshold = 2e-05;
        vec<3> p0 = r.position;
        vec<3> p = p0;
        auto d0 = sdf(p0);
        auto d = d0;
        auto t = 0.f;
        auto dir_mag = r.direction.magnitude();

        unsigned i = 3;
        for (; i--;)// && fabsf(d) > stop_threshold;)
        {
            t += d / dir_mag;
            p = p0 + r.direction * t;
            d = sdf(p);
        }

        // if (d > 0)
        // {
        //     auto dw = sdf(p + r.direction);
        //     auto w = dw / (dw - d);
        //     std::cerr << w << std::endl;
        //     //t += w;// / dir_mag;
        // }

        auto d0_sign = (d0 > 0) - (d0 < 0);
        auto d_sign = (d > 0) - (d < 0);
        if (i > 0 || (d0_sign != d_sign))
        {
            auto inter_p = r.point_at(t);

            return {
                t,
                r.position,
                r.direction,
                inter_p,
                g::game::normal_from_sdf(sdf, inter_p)
            };
        }

        return {}; // no intersection
    }

    bool generates_rays() override { return false; }

    std::vector<ray>& rays() override
    {
        ray_list.clear();
        return ray_list;
    }

    const std::vector<intersection>& intersections(collider& other, float max_t = std::numeric_limits<float>::infinity()) override
    {
        intersection_list.clear();
        if (other.generates_rays())
        {
            for (auto& r : other.rays())
            {
                auto i = ray_intersects(r);
                if (i && i.time >= 0 && i.time < max_t) { intersection_list.push_back(i); }
            }
        }
        return intersection_list;
    }

    // ray_collider& operator=(const ray& other)
    // {
    //     position = other.position;
    //     direction = other.direction;
    // }

private:
    std::vector<intersection> intersection_list;
    std::vector<ray> ray_list;
    g::game::sdf sdf;
};


} // end namespace cd


namespace cr //< Collision resolution
{
    /**
     * @brief      Resolves collision for and object which only has translational
     *             degrees of freedom.
     * 
     * @param[inout] obj            The object which we are resolving the collision for.
     * @param[in]    intersections  The intersections.
     *
     * @tparam       T            A type compatible with this function must have 
     *                            `position` and `velocity` vec<3> members
     */
    template<typename T>
    void resolve_linear(T& obj, const std::vector<cd::intersection>& intersections)
    {
        bool printed = false;

        for (auto& intersection : intersections)
        {
            auto n = intersection.normal;

            // allow the velocity components parallel to the surface plane persist
            // which will allow the object to slide along the surface
            obj.velocity = obj.velocity - (n * (std::min<float>(0, obj.velocity.dot(n))));
            auto friction = obj.velocity * -0.3;
            obj.velocity += friction;

            // static coefficent of friction fudge factor
            if (obj.velocity.magnitude() < 0.5) obj.velocity *= 0;

            // correct penetration
            obj.position = intersection.point - (intersection.origin - obj.position);
        } 
    }
} // end namespace cr

} // end namespace dyn
} // end namespace g
