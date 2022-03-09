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

		explicit trait(const char* s)
		{
			type = value_type::string;
			auto len = strlen(s);
			string = new char[len];
			strncpy(string, s, len);
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
					auto len = strlen(o.string);
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
			ryml::Tree tree = ryml::parse_in_place(ryml::to_substr((char*)buffer.data()));

			for (const auto& trait : tree["traits"])
			{
				std::string key(trait.key().str, trait.key().len);
				auto val = trait.val();

				if (!trait.is_val_quoted())
				{ // try to infer type
					char* end_ptr;
					auto f = strtof(val.str, &end_ptr);

					if (end_ptr != val.str)
					{
						_traits[key] = object::trait{ f };
					}
				}
				else
				{ // it's a string
					_traits[key] = object::trait{ val.str };
				}
			}
		}
		else
		{
			// TODO: create
		}
	}

	trait_map& traits() { return _traits; }
private:
	std::string _name;
	trait_map _traits;	
};

} // namespace game

} // namespace g