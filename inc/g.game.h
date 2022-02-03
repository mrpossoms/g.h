#pragma once
#include "xmath.h"
#include <string.h>
#include <stddef.h>
#include <ogt_vox.h>

#include <unordered_set>
#include <vector>
#include <iostream>

#define G_TERM_GREEN "\033[0;32m"
#define G_TERM_RED "\033[1;31m"
#define G_TERM_YELLOW "\033[1;33m"
#define G_TERM_BLUE "\033[1;34m"
#define G_TERM_COLOR_OFF "\033[0m"

using namespace xmath;

namespace g {
namespace game {

struct positionable
{
	virtual ~positionable() {};
	virtual vec<3> position(const vec<3>& pos) = 0;
	virtual vec<3> position() = 0;
};

struct pointable
{
	virtual ~pointable() {};
	virtual vec<3> direction(const vec<3>& dir) = 0;
	virtual vec<3> direction() = 0;
};

struct moveable
{
	virtual ~moveable() {};
	virtual vec<3> velocity(const vec<3>& vel) = 0;
	virtual vec<3> velocity() = 0;
};

struct updateable
{
	virtual void update(float dt, float time) = 0;
};

struct pose
{
	vec<3> position = {0, 0, 0};
	quat<float> orientation = {0, 0, 0, 1};
};

/**
 * Defines a 'signed distance field' used for describing implicit surfaces
 */
using sdf = std::function<float (const vec<3>&)>;

inline vec<3> normal_from_sdf(sdf f, const vec<3>& p, float step=1)
{
    vec<3> grad = {};
    vec<3> deltas[3][2] = {
        {{ step, 0, 0 }, { -step, 0, 0 }},
        {{ 0, step, 0 }, { 0, -step, 0 }},
        {{ 0, 0, step }, { 0,  0, -step }},
    };

    for (int j = 3; j--;)
    {
        vec<3> samples[2];
        samples[0] = p + deltas[j][0];
        samples[1] = p + deltas[j][1];
        grad[j] = f(samples[0]) - f(samples[1]);
    }

    return grad.unit();
}

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
		mat<4, 4>::look_at((position = pos), forward, up);
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
	float near = 0.1f, far = 1000.f;
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

struct fps_camera final : public camera_perspective, updateable
{
	//~fps_camera() = default;

	//vec<3> velocity() override { return vel; }
	//vec<3> velocity(const vec<3>& v) override { return vel = v; }

	vec<3> velocity = {};
	vec<3> l = { 1, 0, 0 };
	vec<3> u = { 0, 1, 0 };
	vec<3> f = { 0, 0, 1};


	mat<3, 3> basis = {
		{ 1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, 0, 1 },
	};

	quat<> q, r;

	float yaw = 0;
	float pitch = 0;
	const float max_pitch = M_PI / 2.f;
	const float min_pitch = -M_PI / 2.f;

	vec<3> forward() const override 
	{
		//return basis[2];
		return (q * r).rotate({ 0, 0, -1 });
		return f;
	}

	vec<3> left() const override
	{
		//return basis[0];
		return (q * r).rotate({ 1, 0, 0 });
		return l;
	}

	vec<3> up() const override
	{
		//return basis[1];
		return q.rotate({ 0, 1, 0 });
		return u;
	}


	void update(float dt, float t) override
	{
		pitch = std::min<float>(max_pitch, std::max<float>(min_pitch, pitch));

		//forward = vec<3>::cross(up, right);

		//orientation = quat<>::from_matrix({
		//	{ right[0], right[1], right[2], 0 },
		//	{ up[0], up[1], up[2], 0 },
		//	{ forward[0], forward[1], forward[2], 0 },
		//	{0, 0, 0, 1}
		//}).inverse();

		r = quat<>::from_axis_angle(left(), pitch) * quat<>::from_axis_angle(up(), yaw);
		orientation = r * q;
		//basis[2] = vec<3>::cross(up(), left());
		//basis[0] = vec<3>::cross(up(), forward());

		f = vec<3>::cross(u, l);
		l = vec<3>::cross(f, u);

		// orientation *= q;//quat<>::from_matrix(basis);
		
		// look_at(subject, up);

		position += velocity * dt;
	}
private:
	//vec<3> vel = {};
};

template<typename DAT>
struct voxels
{
	struct itr
	{
		struct voxel
		{
			size_t x, y, z;
			DAT* vox;

			voxel(DAT* vox, size_t x, size_t y, size_t z)
			{
				this->vox = vox;
				this->x = x; this->y = y; this->z = z;
			}
		};

		size_t width, height, depth;
		size_t offset, max_offset;
		DAT* v;

		itr(DAT* voxels, size_t w, size_t h, size_t d, bool end)
		{
			v = voxels;
			width = w; height = h; depth = d;
			offset = 0;
			max_offset = (width * height * depth) - 1;

			if (end)
			{
				offset = max_offset;
			}
		}

		void operator++()
		{
			offset++;
			offset = std::min(offset, max_offset);
		}

		bool operator!=(itr& it)
		{
			return it.offset != offset;
		}

		voxel operator*()
		{
			return { v + offset, offset / (height * depth), (offset / depth) % height, offset % depth, };
		}
	};

	struct slice
	{
		size_t depth = 0;
		DAT* v = nullptr;

		slice(DAT* ptr, size_t d)
		{
			v = ptr;
			depth = d;
		}

		DAT* operator[](size_t idx_h)
		{
			return v + (idx_h * depth);
		}
	};

	size_t width, height, depth;
	std::vector<DAT> v;
	vec<3> com;

	voxels() = default;

	voxels(size_t w, size_t h, size_t d)
	{
		width = w;
		height = h;
		depth = d;
		v.resize(w * h * d);
	}

	voxels(const DAT* ptr, size_t w, size_t h, size_t d)
	{
		width = w;
		height = h;
		depth = d;
		v.resize(w * h * d);
		memcpy(v.data(), ptr, sizeof(DAT) * w * h * d);
	}

	void resize(size_t w, size_t h, size_t d)
	{
		width = w;
		height = h;
		depth = d;
		v.resize(w * h * d);
	}

	uint32_t hash()
	{
		auto data_len = width * height * depth * sizeof(DAT);
		uint32_t* data = static_cast<uint32_t*>(v.data());
		uint32_t h = 0;

	    const uint32_t m = 0x5bd1e995;
	    while (data_len >= 4)
	    {
	        uint32_t k = data[0];
	        k *= m;
	        k ^= k >> (signed)24;
	        k *= m;
	        h *= m;
	        h ^= k;
	        data++;
	        data_len -= 4;
	    }

	    return h;
	}

	vec<3> center_of_bounds()
	{
		return { (float)width / 2, (float)height / 2, (float)depth / 2};
	}

	vec<3>& center_of_mass(bool recompute=false)
	{
		if (recompute)
		{
			com = {0, 0, 0};
			auto count = 0;
			for (size_t w = 0; w < width; w++)
			for (size_t h = 0; h < height; h++)
			for (size_t d = 0; d < depth; d++)
			{
				auto vox = this->idx2(w, h, d);
				if (vox)
				{
					com += {(float)w + 0.5f, (float)h + 0.5f, (float)d + 0.5f};
					count += 1;
				}
			}

			com /= count;
			//com += vec<3>{0.5f, 0.5f, 0.5};
		}

		return com;
	}

	void find(const DAT& needle, std::function<void(size_t x, size_t y, size_t z)> found_cb)
	{
		for (size_t w = 0; w < width; w++)
		for (size_t h = 0; h < height; h++)
		for (size_t d = 0; d < depth; d++)
		{
			if (this->idx(w, h, d) == needle)
			{
				found_cb(w, h, d);
			}
		}
	}

	inline const DAT& idx(size_t x, size_t y, size_t z) const
	{
		return v.at((x * height * depth) + (y * depth) + z);
	}

	inline DAT& idx(size_t x, size_t y, size_t z)
	{
		return v[(x * height * depth) + (y * depth) + z];
	}

	inline DAT& idx2(size_t x, size_t y, size_t z) const
	{
		return v.at(x + (y * width) + (z * width * height));
	}

	inline DAT& idx2(size_t x, size_t y, size_t z)
	{
		return v[x + (y * width) + (z * width * height)];
	}

	slice operator[](size_t idx_w)
	{
		return { v + (idx_w * height * depth), depth };
	}

	itr begin() { return itr(v, width, height, depth, false); }
	itr end() { return itr(v, width, height, depth, true); }
};

struct voxels_paletted : public voxels<uint8_t>
{
	ogt_vox_palette palette;

	voxels_paletted() = default;

	voxels_paletted(const ogt_vox_palette& pal, const uint8_t* ptr, size_t w, size_t h, size_t d) : voxels(ptr, w, h, d)
	{
		palette = pal;
	}

	voxels_paletted(const voxels_paletted& vox, std::unordered_set<uint8_t> exclude) : voxels(vox.width, vox.height, vox.depth)
	{
		sift(vox, exclude);
	}

	void sift(const voxels_paletted& src, std::unordered_set<uint8_t> exclude)
	{
		palette = src.palette;
		this->resize(src.width, src.height, src.depth);
		for (size_t w = 0; w < width; w++)
		for (size_t h = 0; h < height; h++)
		for (size_t d = 0; d < depth; d++)
		{
			auto sv = src.idx(w, h, d);

			// copy only voxels that are not marked as excluded
			if (exclude.find(sv) == exclude.end())
			{
				this->idx(w, h, d) = sv;
			}
		}
	}
};

} // end namespace game
} // end namespace g
