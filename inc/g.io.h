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
		bool write = false;
		bool read = true;
		bool truncate = false;
	};

	file(const std::string& path, const mode& mode);
	file(const char* path, const mode& mode);
	~file();

	file(file&& o) noexcept;
	file& operator=(file&& o);

	size_t size();

	void read(void* buf, size_t bytes);
	std::vector<uint8_t> read(size_t bytes);

	void write(void* buf, size_t bytes);
	void write(const std::vector<uint8_t>& buf);

	void seek(size_t byte_position);

	void on_changed(std::function<void(file&)> callback);

private:
	std::unique_ptr<impl> file_impl;
};

} // namespace io
} // namespace g