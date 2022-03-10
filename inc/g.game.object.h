#pragma once
#include <unordered_map>
#include <stdlib.h>

#include <g.io.h>
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
	object(const std::string& name, const trait_map traits) : _name(name), _traits(traits)
	{
		auto f = g::io::file(name);

		if (f.exists())
		{
			auto buffer = f.read_all();
			ryml::Tree tree = ryml::parse_in_place(ryml::substr((char*)buffer.data(), buffer.size()));

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

	trait_map& traits() { return _traits; }
private:
	std::string _name;
	trait_map _traits;	
};

} // namespace game

} // namespace g