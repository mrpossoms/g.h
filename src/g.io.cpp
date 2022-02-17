#include "g.io.h"

#include <sys/types.h>
#include <fcntl.h>


#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <sys/stat.h>
#elif __linux__
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#elif _WIN32
#include <io.h>
#define open _open
#define lseek _lseek
#endif


struct g::io::file::impl
{
	static int in_fd; // inotify instance file desc
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

		// if (impl::in_fd < 0)
		// {
		// 	impl::in_fd = inotify_init();
		// }

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
		auto last = ::lseek(fd, 0, SEEK_CUR);
		auto end = ::lseek(fd, 0, SEEK_END);
		::lseek(fd, last, SEEK_SET);

		return end;
	}

	int read(void* buf, size_t bytes)
	{
		return ::read(fd, buf, bytes);
	}

	std::vector<uint8_t> read(size_t bytes)
	{
		std::vector<uint8_t> buf;
		buf.reserve(bytes);

		auto size = ::read(fd, buf.data(), bytes);
		buf.resize(size);

		return buf;
	}

	int write(void* buf, size_t bytes)
	{
		return ::write(fd, buf, bytes);
	}

	int write(const std::vector<uint8_t>& buf)
	{
		return ::write(fd, buf.data(), buf.size());
	}

	void seek(size_t byte_position)
	{
		::lseek(fd, byte_position, SEEK_SET);
	}

	time_t modified()
	{
		struct stat stat_buf = {};
		fstat(fd, &stat_buf);

#ifdef __linux__
		return stat_buf.st_mtim.tv_sec;
#elif _WIN32
		return stat_buf.st_mtime;
#endif

	}

	void on_changed(std::function<void(file&)> callback){}
};

g::io::file::file(const std::string& path, const mode& mode)
{
	file_impl = std::make_unique<g::io::file::impl>(path.c_str(), mode);
}

g::io::file::file(const char* path, const mode& mode)
{

}

g::io::file::~file()
{

}

g::io::file::file(file&& o) noexcept : file_impl(std::move(o.file_impl))
{

}

g::io::file& g::io::file::operator=(file&& o)
{
	file_impl.swap(o.file_impl);

	return *this;
}

size_t g::io::file::size()
{
	return file_impl->size();
}

int g::io::file::read(void* buf, size_t bytes)
{
	return file_impl->read(buf, bytes);
}

std::vector<uint8_t> g::io::file::read(size_t bytes)
{
	return file_impl->read(bytes);
}

int g::io::file::write(void* buf, size_t bytes)
{
	return file_impl->write(buf, bytes);
}

int g::io::file::write(const std::vector<uint8_t>& buf)
{
	return file_impl->write(buf);
}

void g::io::file::seek(size_t byte_position)
{
	file_impl->seek(byte_position);
}

time_t g::io::file::modified()
{
	return file_impl->modified();
}

void g::io::file::on_changed(std::function<void(g::io::file&)> callback)
{

}