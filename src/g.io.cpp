#include "g.io.h"

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>

struct g::io::file::impl
{
	int fd = -1;

	impl(const char* path, const mode& mode)
	{
		int flags = 0;

		if (mode.write && mode.read)
		{
			flags |= O_RDWR;	
		}
		else if (mode.read)
		{
			flags |= O_RDONLY;
		}
		else if (mode.write)
		{
			flags |= O_WRONLY;
		}

		if (mode.write)
		{
			flags |= O_CREAT;

			if (mode.truncate)
			{
				flags |= O_TRUNC;
			}
			else
			{
				flags |= O_APPEND;
			}
		}

		fd = open(path, flags, 0x666);
	}

	~impl()
	{
		if (fd > -1)
		{
			close(fd);
			fd = -1;
		}
	}

	size_t size()
	{
		auto last = lseek(fd, 0, SEEK_CUR);
		auto end = lseek(fd, 0, SEEK_END);
		lseek(fd, last, SEEK_SET);

		return end;
	}

	void read(void* buf, size_t bytes)
	{
		read(fd, buf, bytes);
	}

	std::vector<uint8_t> read(size_t bytes)
	{
		std::vector<uint8_t> buf;
		buf.reserve(bytes);

		auto size = read(fd, buf.data(), bytes);
		buf.resize(size);

		return buf;
	}

	void write(void* buf, size_t bytes)
	{
		write(fd, buf, bytes);
	}

	void write(const std::vector<uint8_t>& buf)
	{
		write(fd, buf.data(), buf.size());
	}

	void seek(size_t byte_position)
	{
		lseek(fd, byte_position, SEEK_SET);
	}

	void on_changed(std::function<void(file&)> callback)
	{

	}
};
#endif

g::io::file::file(const std::string& path, const mode& mode);
g::io::file::file(const char* path, const mode& mode);
g::io::file::~file();

file(file&& o) noexcept;
file& operator=(file&& o);

size_t size();

void read(void* buf, size_t bytes);
std::vector<uint8_t> read(size_t bytes);

void write(void* buf, size_t bytes);
void write(const std::vector<uint8_t>& buf);

void seek(size_t byte_position);

void on_changed(std::function<void(file&)> callback);