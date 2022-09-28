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


struct file
{
	struct impl;

	struct mode
	{
		mode() = default;

		bool _write = false;
		bool _read = false;
		bool _truncate = false;

		mode& write(bool f) { _write = f; return *this; }
		mode& read(bool f) { _read = f; return *this; }
		mode& truncate(bool f) { _truncate = f; return *this; }

		static mode read_only() { return file::mode{}.read(true); }
		static mode write_only() { return file::mode{}.write(true); }
	};

	file(const std::string& path, const mode& mode=mode::read_only());
	file(const char* path, const mode& mode=mode::read_only());
	~file();

	file(file&& o) noexcept;
	file& operator=(file&& o);

	size_t size();

	int read(void* buf, size_t bytes);
	std::vector<uint8_t> read(size_t bytes);
	std::vector<uint8_t> read_all();

	int write(const void* buf, size_t bytes);
	int write(const std::vector<uint8_t>& buf);

	void seek(size_t byte_position);

	time_t modified();

	void on_changed(std::function<void(file&)> callback);

	bool exists() const;

	static void make_path(const char* path);

	int get_fd() const;
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