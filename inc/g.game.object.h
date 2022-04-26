#pragma once
#include <unordered_map>
#include <stdlib.h>
#include <variant>

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
	// TODO: replace with std::variant
	/*struct trait
	{
		enum class value_type
		{
			none,
			number,
			string,
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
	};*/


	using trait = std::variant<std::string, float, double, int>;

	using trait_map = std::unordered_map<std::string, trait>;
	using multi_trait_map = std::unordered_map<std::string, trait_map>;

	object() = default;

	// TODO: move to factory method
	object(g::asset::store* store, 
		   const std::string& name, 
		   const multi_trait_map traits) : _store(store), _name(name), _traits(traits)
	{
		load_if_newer();

		{ // write the object back out
			g::io::file of(name, g::io::file::mode{true, false, true});
			serialize(of);
		}
	}

	void serialize(g::io::file& of)
	{
		ryml::Tree tree;
		ryml::NodeRef root = tree.rootref();
		root |= ryml::MAP;

		for (auto& category_kvp : _traits)
		{
			auto& cat_key = category_kvp.first;
			root[ryml::csubstr(cat_key.c_str(), cat_key.length())] |= ryml::MAP;

			for (auto& val_kvp : category_kvp.second)
			{
				auto& item = val_kvp.second;
				
				if (std::holds_alternative<std::string>(item))
				{
					root[ryml::csubstr(cat_key.c_str(), cat_key.length())].append_child() << ryml::key(val_kvp.first) << std::get<std::string>(item);
				}
				else if (std::holds_alternative<float>(item))
				{
					root[ryml::csubstr(cat_key.c_str(), cat_key.length())].append_child() << ryml::key(val_kvp.first) << std::get<float>(item);
				}

				/*switch(item.type)
				{
					case trait::value_type::number:
						root[ryml::csubstr(cat_key.c_str(), cat_key.length())].append_child() << ryml::key(val_kvp.first) << item.number;
						break;
					case trait::value_type::string:
						root[ryml::csubstr(cat_key.c_str(), cat_key.length())].append_child() << ryml::key(val_kvp.first) << ryml::csubstr(item.string, strlen(item.string));
						break;

				}*/
			}
		}

		auto yml_str = ryml::emitrs<std::string>(tree);
		of.write((void*)yml_str.c_str(), yml_str.length());
	}

	void deserialize(g::io::file& f)
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
					_traits["traits"][key] = object::trait{ float_val };
					continue;
				}
			}

			{ // it's a string
				_traits["traits"][key] = object::trait{ std::string{val.str, val.len} };
			}
		}

		// load textures
		for (const auto& item : tree["textures"])
		{
			if (!item.has_key() || !item.has_val()) { continue; }

			std::string key_str(item.key().str, item.key().len);
			std::string path_str(item.val().str, item.val().len);

			if (_traits.find("textures") == _traits.end()) { _traits["textures"] = trait_map{}; }
			_traits["textures"][key_str] = path_str;

			// if we have a rendering context setup then poke the texture
			// which will cause it to be created if it doesn't exist
			if (g::gfx::has_graphics()) { texture(key_str); }
		}

		// load geometry
		for (const auto& item : tree["geometry"])
		{
			if (!item.has_key() || !item.has_val()) { continue; }

			std::string key_str(item.key().str, item.key().len);
			std::string path_str(item.val().str, item.val().len);

			_traits["geometry"][key_str] = path_str;


			// if we have a rendering context setup then poke the mesh
			// which will cause it to be created if it doesn't exist
			if (g::gfx::has_graphics())
			{
				geometry(key_str);
			}
		}

		// load sounds
		for (const auto& item : tree["sounds"])
		{
			if (!item.has_key() || !item.has_val()) { continue; }

			std::string key_str(item.key().str, item.key().len);
			std::string path_str(item.val().str, item.val().len);

			_traits["sounds"][key_str] = path_str;

			sound(key_str);
		}
	}

	void load_if_newer()
	{
		auto f = g::io::file(_name);

		if (f.exists())
		{
			auto modified = f.modified();

			if (modified > _last_modified)
			{
				_last_modified = f.modified();
				deserialize(f);
			}
		}
	}

	g::gfx::texture& texture(const std::string& name)
	{
		load_if_newer();

		return _store->tex(std::get<std::string>(_traits["textures"][name]), /* make_if_missing = */ true); 
	}

	g::gfx::mesh<g::gfx::vertex::pos_uv_norm>& geometry(const std::string& name)
	{
		load_if_newer();

		return _store->geo(std::get<std::string>(_traits["geometry"][name]), /* make_if_missing = */ true);
	}

	g::snd::track& sound(const std::string& name)
	{
		load_if_newer();

		return _store->sound(std::get<std::string>(_traits["sounds"][name]), /* make_if_missing = */ true);
	}

	trait_map& traits()
	{ 
		load_if_newer();

		return _traits["traits"]; 
	}
private:
	g::asset::store* _store;
	std::string _name;
	multi_trait_map _traits;
	time_t _last_modified = 0;
	// std::unordered_map<std::string, std::string> _texture_map;
	// std::unordered_map<std::string, std::string> _geometry_map;
	// std::unordered_map<std::string, std::string> _sound_map;
};

} // namespace game

} // namespace g