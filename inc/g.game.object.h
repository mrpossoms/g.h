#pragma once
#include <unordered_map>

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
			number,
			string,
			texture,

		};

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

		~trait()
		{
			if (type == value_type::string && string)
			{
				delete[] string;
				string = nullptr;
			}
		}

		value_type type;

		union
		{
			float number;
			char* string;
		};
	};

	using trait_map = std::unordered_map<std::string, trait>;

	// TODO: move to factory method
	object(const std::string& name, const trait_map traits) : _name(name), _traits(traits)
	{
		auto f = g::io::file(name);

		if (f.exists())
		{
			auto buffer = f.read_all();
			ryml::Tree tree = ryml::parse_in_place(ryml::to_substr((char*)buffer.data()));


		}
		else
		{
			// TODO: create
		}
	}

	const std::unordered_map<std::string, trait>& traits() const { return _traits; }
private:
	std::string _name;
	trait_map _traits;	
};

} // namespace game

} // namespace g