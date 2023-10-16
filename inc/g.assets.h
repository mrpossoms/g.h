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
	time_t loaded_time;
	T asset;

	kind() = default;

	kind(time_t last, T a) : asset(std::move(a))
	{
		last_accessed = last;
		loaded_time = last;
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
	private: bool hot_reload;
	private: std::unordered_map<std::string, kind<g::gfx::texture*>> textures;
	private: std::unordered_map<std::string, kind<g::gfx::sprite>> sprites;
	private: std::unordered_map<std::string, kind<g::game::vox_scene>> voxels;
	private: std::unordered_map<std::string, kind<g::gfx::shader*>> shaders;
	private: std::unordered_map<std::string, kind<g::gfx::font>> fonts;
	private: std::unordered_map<std::string, kind<g::gfx::mesh<g::gfx::vertex::pos_uv_norm>>> geos;
	private: std::unordered_map<std::string, kind<g::snd::track>> sounds;

	public: store(const std::string& root_path="data", bool do_hot_reload=true) : root(root_path), hot_reload(do_hot_reload) { }

	~store();

	g::gfx::texture* tex(const std::string& partial_path, bool make_if_missing = false);

	g::gfx::sprite& sprite(const std::string& partial_path, bool make_if_missing = false);

	g::gfx::shader* shader(const std::string& program_collection);

	g::gfx::font& font(const std::string& partial_path, bool make_if_missing = false);

	g::gfx::mesh<g::gfx::vertex::pos_uv_norm>& geo(const std::string& partial_path, bool make_if_missing = false);

	g::game::vox_scene& vox(const std::string& partial_path, bool make_if_missing = false);

	g::snd::track& sound(const std::string& partial_path, bool make_if_missing = false);

	const std::string& get_root_path() const { return root; }
	// g::game::object& game_object(const std::string& partial_path);
};

}

}
