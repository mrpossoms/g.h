#pragma once
#include "xmath.h"
#include <string.h>
#include <stddef.h>
#include <ogt_vox.h>

#ifdef __linux__
#include <GL/glx.h>
#include <GL/glext.h>
#endif

#include <GLFW/glfw3.h>

#ifdef __APPLE__
#undef __gl_h_
#include <OpenGL/gl3.h>
#endif

using namespace xmath;

namespace g {
namespace game {

struct view_point
{
	vec<3> position = {0, 0, 0};
	quat<> orientation = {0, 0, 0, 1};
};


struct camera : public view_point
{
	quat<>& d_pitch(float delta)
	{
		auto dq = quat<>::from_axis_angle({1, 0, 0}, delta);
		return orientation *= dq;
	}

	quat<>& d_yaw(float delta)
	{
		auto dq = quat<>::from_axis_angle({0, 1, 0}, delta);
		return orientation *= dq;
	}

	quat<>& d_roll(float delta)
	{
		auto dq = quat<>::from_axis_angle({0, 0, 1}, delta);
		return orientation *= dq;
	}

	vec<3> forward() const { return orientation.rotate({0, 0, -1}); }

	vec<3> left() const { return orientation.rotate({-1, 0, 0}); }

	vec<3> up() const { return orientation.rotate({0, 1, 0}); }

	mat<4, 4>& look_at(const vec<3>& subject, const vec<3>& up={0, 1, 0})
	{
		return _view = mat<4, 4>::look_at(position * -1.f, (subject + position).unit(), up);
	}

	mat<4, 4>& look_at(const vec<3>& pos, const vec<3>& forward, const vec<3>& up)
	{
		return _view = mat<4, 4>::look_at((position = pos) * -1.0, forward, up);
	}

	mat<4, 4> view() const
	{
		if (_view[3][3] != 0) { return _view; }
		return mat<4, 4>::translation(position * -1) * orientation.to_matrix();
	}

	virtual mat<4, 4> projection() const = 0;

private:
	mat<4, 4> _view = {};
};

struct camera_perspective : public camera
{
	float field_of_view = M_PI / 2;
	float near = 0.1f, far = 1000.f;

	virtual mat<4, 4> projection() const
	{
		GLint vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		auto aspect = vp[2] / (float)vp[3];
		return mat<4, 4>::perspective(near, far, field_of_view, aspect);
	}
};

struct camera_orthographic : public camera
{
	float near = 0.1f, far = 1000.f;
	float width = 10, height = 10;

	virtual mat<4, 4> projection() const
	{
		GLint vp[4];
		glGetIntegerv(GL_VIEWPORT, vp);
		return mat<4, 4>::orthographic(near, far, width/2, -width/2, height/2, -height/2);
	}
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
	DAT* v;
	vec<3> com;

	voxels() = default;

	voxels(size_t w, size_t h, size_t d)
	{
		width = w;
		height = h;
		depth = d;
		v = new DAT[w * h * d];
	}

	voxels(const DAT* ptr, size_t w, size_t h, size_t d)
	{
		width = w;
		height = h;
		depth = d;
		v = new DAT[w * h * d];
		memcpy(v, ptr, sizeof(DAT) * w * h * d);
	}

	uint32_t hash()
	{
		auto data_len = width * height * depth * sizeof(DAT);
		uint32_t* data = static_cast<uint32_t*>(v);
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

	inline DAT& idx(size_t x, size_t y, size_t z) const
	{
		return v[(x * height * depth) + (y * depth) + z];
	}

	inline DAT& idx(size_t x, size_t y, size_t z)
	{
		return v[(x * height * depth) + (y * depth) + z];
	}

	inline DAT& idx2(size_t x, size_t y, size_t z) const
	{
		return v[x + (y * width) + (z * width * height)];
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
};


} // end namespace game
} // end namespace g
