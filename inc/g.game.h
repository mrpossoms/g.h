#pragma once
#include "xmath.h"
#include <string.h>
#include <stddef.h>
#include <ogt_vox.h>
#include <assert.h>

#include <unordered_set>
#include <unordered_map>
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
	virtual void pre_update(float dt, float time) {};
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

template<typename DAT>
struct voxels
{
	struct itr
	{
		struct voxel
		{
			size_t x, y, z;
			vec<3,size_t> size;
			DAT* vox;

			voxel(DAT* vox, size_t x, size_t y, size_t z) : size({x, y, z})
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
	vec<3, size_t> size;
	std::vector<DAT> v;
	vec<3> com;

	voxels() = default;

	voxels(vec<3, int> s) : size(s.cast<size_t>())
	{
		width = s[0];
		height = s[1];
		depth = s[2];
		v.resize(width * height * depth);
	}

	voxels(size_t w, size_t h, size_t d) : size({w, h, d})
	{
		width = w;
		height = h;
		depth = d;
		v.resize(w * h * d);
	}

	voxels(const DAT* ptr, size_t w, size_t h, size_t d) : size({ w, h, d })
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
		size = vec<3, size_t>{ w, h, d };
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

	inline DAT& operator[](const vec<3, size_t>& idx)
	{
		return idx2(idx[0], idx[1], idx[2]);
	}

	itr begin() { return itr(v, width, height, depth, false); }
	itr end() { return itr(v, width, height, depth, true); }
};

struct vox_scene
{
	struct group
	{
		group* parent = nullptr;
		mat<4, 4> transform;
		bool hidden = false;
		// TODO: layer index


	};

	struct model_instance
	{
		mat<4, 4> transform;
		group* group = nullptr;
		voxels<uint8_t>* model;

		mat<4, 4> global_transform() const
		{
			auto T = transform;
			vox_scene::group* g = group;

			while (g != nullptr)
			{
				T *= g->transform;// *T;
				g = g->parent;
			}

			return T;
		}

		std::tuple<vec<3, int>, vec<3, int>> corners(const mat<4, 4>& transform) const
		{
			auto half = model->size.cast<float>() / 2;
			vec<3> m = -half, M = half;

			m = transform * m;
			M = transform * M;

			return { m.cast<int>(), M.ceil().cast<int>() };
		}

		vec<3> position_of(const vec<3>& voxel, bool global = false) const
		{
			auto half = model->size.cast<float>() / 2;
			auto T = transform;			

			if (global)
			{
				T = global_transform() * T;
			}

			return T * (voxel - half);
		} 

		struct mating_options
		{
			// TODO
		};

		bool mate(const model_instance& other, const vec<3, int>& my_voxel, const vec<3, int>& other_voxel, const mating_options& opts={})
		{
			transform = other.transform;
			group = other.group;

			auto delta = other.position_of(other_voxel.cast<float>()) - position_of(my_voxel.cast<float>());
		
			transform[0][0] += delta[0];
			transform[1][0] += delta[1];
			transform[2][0] += delta[2];

			return true;
		}
	};

	ogt_vox_palette palette = {};
	ogt_vox_matl_array materials = {};
	std::vector<voxels<uint8_t>> models;
	std::vector<group> groups;
	std::unordered_map<std::string, model_instance> instances;

	std::tuple<vec<3, int>, vec<3, int>> corners(const group* parent = nullptr) const
	{
		vec<3, int> m = { 0, 0, 0 }, M = { 0, 0, 0 };
		auto first = true;

		for (auto& kvp : instances)
		{
			auto& inst = kvp.second;
			auto T = inst.global_transform();
			std::tuple<vec<3, int>, vec<3, int>> corners = inst.corners(T);

			if (first)
			{
				m = std::get<0>(corners);
				M = std::get<1>(corners);
				first = false;
			}
			else
			{
				m = m.take_min(std::get<0>(corners));
				m = m.take_min(std::get<1>(corners));
				M = M.take_max(std::get<0>(corners));
				M = M.take_max(std::get<1>(corners));
			}
		}

		return { m, M };
	}

	vec<3, size_t> size() const
	{
		auto c = corners();

		return (std::get<1>(c) - std::get<0>(c)).cast<size_t>();
	}

	vec<3, int> center()
	{
		auto c = corners();
		return (std::get<0>(c) + std::get<1>(c)) / 2;
	}

	void duplicate_instance(const model_instance& inst, const std::string& dup_name)
	{
		instances[dup_name] = inst;
	}

	voxels<uint8_t> flatten()
	{
		auto c = corners();
		auto min = std::get<0>(c);
		auto min_f = min.cast<float>();
		vec<3, int> size = std::get<1>(c) - min;

		voxels<uint8_t> out(size);

		for (auto& kvp : instances)
		{
			auto& inst = kvp.second;
			auto T = inst.global_transform();

			std::cout << kvp.first << ": " << inst.model->size.to_string() << std::endl;

			auto half = (inst.model->size.cast<float>() / 2) - 0.5f;

			for (int z = 0; z < inst.model->depth; z++)
			for (int y = 0; y < inst.model->height; y++)
			for (int x = 0; x < inst.model->width; x++)
			{
				auto v = inst.model->idx2(x, y, z);

				if (0 == v) { continue; }

				assert(x < inst.model->width);
				assert(y < inst.model->height);
				assert(z < inst.model->depth);
				auto coord = ((T * (vec<3>{(float)x, (float)y, (float)z} - half)) - min_f);

				assert(coord[0] < size[0]);
				assert(coord[1] < size[1]);
				assert(coord[2] < size[2]);

				out[coord.cast<size_t>()] = v;
			
			}

		}

		return out;
	}

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
