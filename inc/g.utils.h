#pragma once
#include <assert.h>
#include <string>

namespace g {

namespace utils {

template<typename I>
struct inc_at_end
{
	I& incer;
	inc_at_end(I& i) : incer(i) {}
	~inc_at_end() { incer++; }
};

struct split
{
public:
	struct it
	{
	public:
		it(const std::string &str, std::string delim, size_t pos);

		void operator++();

		bool operator!=(it &i);

		std::string operator*();

	protected:
		const std::string& _str;
		std::string _delim;
		size_t _pos, _next_pos;
	};

	/**
	 * @brief iterable class that splits a string into tokens
	 *        separated by occurences of delim
	 * @param str String whose tokens we want to iterate over.
	 * @param delim String used as delimiter to create tokens.
	 */
	split(const std::string &str, std::string delim);

	it begin();

	it end();

private:
	const std::string& _str;
	std::string _delim;
};


// std::string base64_encode(uint8_t const* buf, size_t len);
void base64_encode(void *dst, const void *src, size_t len); // thread-safe, re-entrant

}

}