#pragma once
#include <unordered_map>
#include <stdlib.h>

#include <g.io.h>
#include <g.assets.h>
#include <ryml_std.hpp>
#include <ryml.hpp>

namespace g
{
namespace game
{

struct object
{
	struct trait
	{
		enum class value_type
		{
			none,
			number,
			string,
			texture,

		};

		trait()
		{
			type = value_type::none;
		}

		trait(float f)
		{
			type = value_type::number;
			number = f;
		}

		trait(const char* s)
		{
			type = value_type::string;
			auto len = strlen(s) + 1;
			string = new char[len];
			strncpy(string, s, len);
		}

		trait(char* s)
		{
			type = value_type::string;
			auto len = strlen(s) + 1;
			string = new char[len];
			strncpy(string, s, len);
		}

		trait(const std::string& s)
		{
			type = value_type::string;
			string = new char[s.length() + 1];
			strncpy(string, s.c_str(), s.length() + 1);
		}

		trait(const trait& o)
		{
			type = o.type;

			switch(type)
			{
				case value_type::number:
					number = o.number;
					break;
				case value_type::string:
				{
					auto len = strlen(o.string) + 1;
					string = new char[len];
					strncpy(string, o.string, len);
				} break;
				default:
					break;
			}
		}

		trait(trait&& o)
		{
			free_if_string();

			type = o.type;

			switch(type)
			{
				case value_type::number:
					number = o.number;
					break;
				case value_type::string:
					string = o.string;
					o.string = nullptr;
					break;
				default:
					break;
			}
		}

		void operator=(trait&& o)
		{
			free_if_string();

			type = o.type;

			switch(type)
			{
				case value_type::number:
					number = o.number;
					break;
				case value_type::string:
					string = o.string;
					o.string = nullptr;
					break;
				default:
					break;
			}
		}

		void free_if_string()
		{
			if (type == value_type::string && string)
			{
				delete[] string;
				string = nullptr;
			}			
		}

		~trait()
		{
			free_if_string();
		}

		value_type type;

		union
		{
			float number;
			char* string;
		};
	};

	using trait_map = std::unordered_map<std::string, object::trait>;

	// TODO: move to factory method
	object(g::asset::store* store, const std::string& name, const trait_map traits) : _store(store), _name(name), _traits(traits)
	{
		auto f = g::io::file(name);

		if (f.exists())
		{
			auto buffer = f.read_all();
			ryml::Tree tree = ryml::parse_in_place(ryml::substr((char*)buffer.data(), buffer.size()));

			// load traits
			for (const auto& trait : tree["traits"])
			{
				std::string key(trait.key().str, trait.key().len);
				auto val = trait.val();

				if (!trait.is_val_quoted())
				{ // try to infer type
					char* end_ptr;
					auto float_val = strtof(val.str, &end_ptr);

					if (end_ptr != val.str)
					{
						_traits[key] = object::trait{ float_val };
						continue;
					}
				}

				{ // it's a string
					_traits[key] = object::trait{ std::string{val.str, val.len} };
				}
			}

			// load textures
			for (const auto& item : tree["textures"])
			{
				if (!item.has_key() || !item.has_val()) { continue; }

				std::string key_str(item.key().str, item.key().len);
				std::string path_str(item.val().str, item.val().len);

				_texture_map[key_str] = path_str;

				texture(key_str);
			}

			// load geometry
			for (const auto& item : tree["geometry"])
			{
				if (!item.has_key() || !item.has_val()) { continue; }

				std::string key_str(item.key().str, item.key().len);
				std::string path_str(item.val().str, item.val().len);

				_geometry_map[key_str] = path_str;
			}

			// load sounds
			for (const auto& item : tree["sounds"])
			{
				if (!item.has_key() || !item.has_val()) { continue; }

				std::string key_str(item.key().str, item.key().len);
				std::string path_str(item.val().str, item.val().len);

				_sound_map[key_str] = path_str;
			}
		}
		else
		{
			g::io::file of(name, g::io::file::mode::write_only());

			ryml::Tree tree;
			ryml::NodeRef root = tree.rootref();
			root |= ryml::MAP;
			root["traits"] |= ryml::MAP;
			for (auto& kvp : _traits)
			{
				auto& trait = kvp.second;
				switch(trait.type)
				{
					case trait::value_type::number:
						root["traits"].append_child() << ryml::key(kvp.first) << trait.number;
						break;
					case trait::value_type::string:
						root["traits"].append_child() << ryml::key(kvp.first) << ryml::csubstr(trait.string, strlen(trait.string));
						break;

				}
			}

			auto yml_str = ryml::emitrs<std::string>(tree);
			of.write((void*)yml_str.c_str(), yml_str.length());
		}
	}

	g::gfx::texture& texture(const std::string& partial_path) { return _store->tex(partial_path, /* make_if_missing = */ true); }

	g::gfx::mesh<g::gfx::vertex::pos_uv_norm>& geometry(const std::string& partial_path) { return _store->geo(partial_path, /* make_if_missing = */ true); }

	g::snd::track& sound(const std::string& partial_path) { return _store->sound(partial_path, /* make_if_missing = */ true); }

	trait_map& traits() { return _traits; }
private:
	g::asset::store* _store;
	std::string _name;
	trait_map _traits;
	std::unordered_map<std::string, std::string> _texture_map;
	std::unordered_map<std::string, std::string> _geometry_map;
	std::unordered_map<std::string, std::string> _sound_map;
};

} // namespace game

} // namespace g