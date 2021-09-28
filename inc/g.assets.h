#pragma once

#include <time.h>
#include <unordered_map>
#include <string>

#include "g.gfx.h"
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

	kind(time_t last, T a)
	{
		last_accessed = last;
		asset = a;
	}

	T& get()
	{
		last_accessed = time(nullptr);
		return asset;
	}
};

struct store
{
	private: std::string root;
	private: std::unordered_map<std::string, kind<g::gfx::texture>> textures;
	private: std::unordered_map<std::string, kind<g::game::voxels_paletted>> voxels;
	private: std::unordered_map<std::string, kind<g::gfx::shader>> shaders;
	private: std::unordered_map<std::string, kind<g::gfx::font>> fonts;

	public: store(const std::string& root_path="data") : root(root_path) { }

	g::gfx::texture& tex(const std::string& partial_path);

	g::gfx::shader& shader(const std::string& program_collection);

	g::gfx::font& font(const std::string& partial_path);

	g::game::voxels_paletted& vox(const std::string& partial_path);
};

}

}
