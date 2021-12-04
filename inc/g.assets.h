#pragma once

#include <time.h>
#include <unordered_map>
#include <string>

#include "g.gfx.h"
#include "g.snd.h"
#include "g.game.h"

namespace g
{

namespace asset
{

template<typename T>
struct kind
{
	time_t last_accessed;
	T asset;

	kind() = default;

	kind(time_t last, T a) : asset(std::move(a))
	{
		last_accessed = last;
	}

	T& get()
	{
		last_accessed = time(nullptr);
		return asset;
	}
};

using namespace g::gfx;

struct store
{
	private: std::string root;
	private: std::unordered_map<std::string, kind<texture>> textures;
	private: std::unordered_map<std::string, kind<g::game::voxels_paletted>> voxels;
	private: std::unordered_map<std::string, kind<shader>> shaders;
	private: std::unordered_map<std::string, kind<font>> fonts;
	private: std::unordered_map<std::string, kind<mesh<vertex::pos_uv_norm>>> geos;
	private: std::unordered_map<std::string, kind<g::snd::track>> sounds;

	public: store(const std::string& root_path="data") : root(root_path) { }

	g::gfx::texture& tex(const std::string& partial_path);

	g::gfx::shader& shader(const std::string& program_collection);

	g::gfx::font& font(const std::string& partial_path);

	g::gfx::mesh<vertex::pos_uv_norm>& geo(const std::string& partial_path);

	g::game::voxels_paletted& vox(const std::string& partial_path);

	g::snd::track& sound(const std::string& partial_path);
};

}

}
