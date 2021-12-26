#pragma once

#define XMTYPE float
#include <xmath.h>

#ifdef __unix__
#include <unistd.h>
#include <strings.h>
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <initializer_list>
#include <functional>
#include <thread>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "g.utils.h"
#include "g.gfx.h"
#include "g.game.h"
#include "g.assets.h"
#include "g.dyn.h"
#include "g.ui.h"
#include "g.ai.h"
#include "g.snd.h"
#ifndef __EMSCRIPTEN__
#include "g.net.h"
#endif

namespace g {


struct core
{
	struct opts
	{
		enum class render_api
		{
			OPEN_GL,
		};

		struct render_api_version
		{
			unsigned int major, minor;
		};

		const char* name;
		struct {
			bool display = true;
			size_t width = 640;
			size_t height = 480;
			bool fullscreen = false;
			render_api api = render_api::OPEN_GL;
			render_api_version api_version = { 4, 1 };
		} gfx;

		opts() = default;

		opts(const char* name, bool display, size_t width, size_t height)
		{
			this->name = name;
			gfx.display = display;
			gfx.width = width;
			gfx.height = height;
		}
	};

	/**
	 * @brief      Overriding the initialize function gives you the chance
	 * to run setup and initalization code before the update loop begins.
	 *
	 * @return     true if the initialization completed successfully, false
	 * otherwise.
	 */
	virtual bool initialize () { return true; }

	/**
	 * @brief      { function_description }
	 */
	void tick();

	/**
	 * @brief      The update function is effectively the main loop of your
	 * game. Override this to run your game logic every frame/tick
	 *
	 * @param[in]  dt    Amount of time elapsed since the previous call to
	 * update had finished.
	 */
	virtual void update (float dt) { }

	/**
	 * @brief      Calling start will first call initialize, do any additional
	 * setup that might have been indicated by the opts parameter, then starts
	 * the update loop until the `running` flag is set to false.
	 *
	 * @param[in]  opts  Special configuration options. See the declaration of
	 * core::opts for more details.
	 */
	void start(const core::opts& opts);

	std::chrono::system_clock::time_point t_1 = std::chrono::system_clock::now();
	volatile bool running = true;
};

/**
 * @brief Simple, very low overhead datastructure for storing unordered data.
 * @tparam T Type whose instances will be stored in the bounded_list.
 * @tparam CAP Maximum number of elements that may be stored in the list.
 */
template<typename T, size_t CAP>
struct bounded_list
{
	struct itr
	{
		itr(T* data, size_t pos, size_t len) { _data = data; _pos = pos; _len = len; }

		bool operator!=(const itr& o) { return _data != o._data || _pos != o._pos || _len != o._len; }
		itr& operator++() { _pos++; return *this; }
		T& operator*() { return _data[_pos]; }

	protected:
		T* _data = nullptr;
		size_t _pos = 0;
		size_t _len = 0;
	};

	bounded_list()
	{
		_size = 0;
	}

	bounded_list(std::initializer_list<T> items)
	{
		_size = 0;
		for (auto item : items)
		{
			push_back(item);
		}
	}

	bool emplace_back()
	{
		if (_size >= CAP) { return false; }

		_size++;

		return true;
	}

	/**
	 * @brief Append element to the end of the list.
	 * @param e Element to add to the bounded_list
	 * @return True if the bounded_list has enough space, False if there
	 *         wasn't enough space.
	 */
	bool push_back(T e)
	{
		if (_size >= CAP) { return false; }

		_list[_size++] = e;

		return true;
	}

	/**
	 * @brief Remove the last element from the list.
	 * @param e Set to the value of the element removed.
	 * @return False if the list is empty, True otherwise.
	 */
	bool pop_back(T& e)
	{
		if (_size <= 0) { return false; }

		e = _list[--_size];

		return true;
	}

	T* add()
	{
		if (_size < CAP)
		{
			return &_list[_size++];
		}


		return nullptr;
	}

	/**
	 * @brief Remove the last element from the list.
	 * @return False if the list is empty, True otherwise.
	 */
	bool pop_back()
	{
		if (_size <= 0) { return false; }

		--_size;

		return true;
	}

	/**
	 * @brief Get the last element of the list.
	 * @param e Set to the value of the last element.
	 * @return False if the list is empty, True otherwise.
	 */
	bool peek_back(T& e)
	{
		if (_size <= 0) { return false; }

		e = _list[_size - 1];

		return true;
	}

	/**
	 * @brief Get the last element of the list.
	 * @return Reference to the last element in the list.
	 * @note Invokes undefined behavior for empty lists.
	 */
	T& peek_back() { return _list[_size - 1]; }

	const T& peek_back_c() const { return _list[_size - 1]; }

	/**
	 * @brief Removes an element at a specific index.
	 * @param idx Index to element to remove from the list.
	 * @return False if the list is empty, or idx is outside of the
	 *         bounds True otherwise.
	 */
	bool remove_at(size_t idx)
	{
		if (idx >= _size) { return false; }
		if (_size == 0) { return false; }

		_list[idx] = _list[_size - 1];
		_size--;

		return true;
	}

	/**
	 * @brief Removes an element from the list.
	 * @return True if a matching element is found in the list
	 *         otherwise False.
	 * @note In the event of duplicate entries, only the first occurance
	 *       is removed.
	 */
	bool remove(T e)
	{
		for (size_t i = 0; i < _size; i++)
		{
			if (e == _list[i]) { return remove_at(i); }
		}

		return false;
	}

	/**
	 * @brief      Clears all elements from the object.
	 */
	void clear()
	{
		_size = 0;
	}

	/**
	 * @return Number of elements contained by the list.
	 */
	size_t size() const { return _size; }


	/**
	 * @brief      Get underlying storage array for the list
	 *
	 * @return     Return raw pointer to the start of the list
	 */
	T* list() { return _list; }

	itr begin() { return itr(_list, 0, _size); }
	itr end() { return itr(_list, _size, _size); }

	inline T& operator[](size_t idx) { return _list[idx]; }

	inline T& item(size_t idx) { return _list[idx]; }

	inline const T& item_c(size_t idx) const { return _list[idx]; }

	inline T* item_ptr(size_t idx) { return &_list[idx]; }

	inline T item(size_t idx) const { return _list[idx]; }
// private:
	T _list[CAP];
	size_t _size;
};

} // namespace g
