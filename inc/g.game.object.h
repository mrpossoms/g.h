#pragma once
#include <g.io.h>

namespace g
{
namespace game
{

template<typename T>
struct object
{
	struct trait
	{
		enum value_type
		{
			number,
			string
		};

		trait(float f)
		{
			type = value_type::number;
			number = f;
		}

		trait(const char* s) explicit
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
			const char* string;
		}
	};

	using trait_map = std::unordered_map<std::string, trait>;

	object(const std::string& name, const trait_map traits) : _name(name), _traits(traits)
	{
		auto f = g::io::file(name);

		if (f.exists())
		{
			// TODO: load
		}
		else
		{
			// TODO: create
		}
	}

	void operator()(T& recipient) const { recipient.map_traits(_traits); }
	const std::unordered_map<std::string, trait>& traits() const { return _traits; }
private:
	std::string _name;
	trait_map _traits;	
};

} // namespace game

} // namespace g