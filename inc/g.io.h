#pragma once

#include <stddef.h>

#include <string>
#include <vector>
#include <memory>
#include <functional>


namespace g
{
namespace io
{

// template
struct file
{
	struct impl;

	struct mode
	{
		mode() = default;

		bool write = false;
		bool read = true;
		bool truncate = false;

		static mode read_only() { return mode{}; }
	};

	file(const std::string& path, const mode& mode=mode::read_only());
	file(const char* path, const mode& mode=mode::read_only());
	~file();

	file(file&& o) noexcept;
	file& operator=(file&& o);

	size_t size();

	int read(void* buf, size_t bytes);
	std::vector<uint8_t> read(size_t bytes);

	int write(void* buf, size_t bytes);
	int write(const std::vector<uint8_t>& buf);

	void seek(size_t byte_position);

	time_t modified();

	void on_changed(std::function<void(file&)> callback);

	bool exists() const ;
private:
	std::unique_ptr<impl> file_impl;
};

// TODO
struct keyboard
{
	static bool is_pressed(int key) { return false; }
};

} // namespace io
} // namespace g