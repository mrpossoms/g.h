#include "g.io.h"

#include <sys/types.h>
#include <fcntl.h>

#include <filesystem>

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <sys/stat.h>
#elif __linux__
#include <unistd.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <string.h>
#include <dirent.h>
#elif _WIN32
#include <io.h>
#include <direct.h>
#ifndef _S_ISTYPE
#define _S_ISTYPE(mode, mask)  (((mode) & _S_IFMT) == (mask))
#define S_ISREG(mode) _S_ISTYPE((mode), _S_IFREG)
#define S_ISDIR(mode) _S_ISTYPE((mode), _S_IFDIR)
#endif
#define open _open
#define lseek _lseek
#define mkdir _mkdir
#define PATH_MAX _MAX_PATH
#elif __EMSCRIPTEN__
#include <emscripten.h>
#endif

struct g::io::path::itr::impl
{
	impl(const g::io::path::impl& p) : internal_itr{p.path}
	{ }

	~impl()
	{

	}

	g::io::path& operator*()
	{
		return curr_path;
	}

	g::io::path* operator->()
	{
		return &curr_path;
	}

	impl& operator++()
	{
		auto dir = *internal_itr;
		// TODO: deal with modes
		curr_path = g::io::path{ dir.path().generic_string() };
		internal_itr++;
		return *this;
	}
	impl& operator++(int)
	{
		internal_itr++;
		auto dir = *internal_itr;
		// TODO: deal with modes
		curr_path = g::io::path{ dir.path().generic_string() };
		return *this;
	}

	bool operator==(const impl& o)
	{
		return internal_itr == o.internal_itr;
	}

	bool operator!=(const impl& o)
	{
		return internal_itr != o.internal_itr;
	}

protected:
	g::io::path curr_path;
	std::filesystem::directory_iterator internal_itr;
};

struct g::io::path::impl
{
	impl() = default;

	impl(const char* path, const mode& mode) : path(path), path_mode(mode)
	{

	}

	impl(const std::string& path, const mode& mode) : path(path), path_mode(mode)
	{

	}

private:
	mode path_mode;
	std::filesystem::path path;

	friend class g::io::path::itr::impl;
};

g::io::path::path(const char* path, const mode& mode = mode::read_only)
{

}

g::io::path::path(const std::string& path, const mode& mode = mode::read_only())
{

}

const std::string& g::io::path::str() const
{

}

g::io::path::itr g::io::path::begin()
{

}

g::io::path::itr g::io::path::end()
{

}

struct g::io::file::impl
{
	static int in_fd; // inotify instance file desc
	std::string path_str;
	mode file_mode;
	int fd = -1;

	impl(const char* path, const mode& mode) : path_str(path), file_mode(mode)
	{
		open_fds();
	}

	impl(impl& o)
	{
		path_str = o.path_str;
		file_mode = o.file_mode;
		open_fds();
	}

	~impl()
	{
		if (fd > -1)
		{
			close(fd);
			fd = -1;
		}
	}

	static void make_path(const char* path)
	{
		char path_str[PATH_MAX];

		for (unsigned i = 0; i < strlen(path); i++)
		{
			if (path[i] == '/')
			{
				strncpy(path_str, path, i);
				path_str[i] = '\0';

#ifdef _WIN32
				mkdir(path_str);
#else
				mkdir(path_str, 0777);
#endif
			}
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
		buf.resize(bytes);

		auto size = ::read(fd, buf.data(), bytes);
		buf.resize(size);

		return buf;
	}

	int write(const void* buf, size_t bytes)
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
#elif __APPLE__
		return stat_buf.st_mtimespec.tv_sec;
#elif _WIN32
		return stat_buf.st_mtime;
#endif

	}

	void on_changed(std::function<void(file&)> callback){}

	bool exists() { return fd >= 0; }

	bool is_directory() const
	{
		struct stat stat_buf = {};
		fstat(fd, &stat_buf);

	
		return S_ISDIR(stat_buf.st_mode);
	}

	int get_fd() const { return fd; }

private:

	void open_fds()
	{
		int flags = 0;

		if (file_mode._write && file_mode._read)
		{
			flags |= O_RDWR;
		}
		else if (file_mode._read)
		{
			flags |= O_RDONLY;
		}
		else if (file_mode._write)
		{
			flags |= O_WRONLY;
		}

		if (file_mode._write)
		{
			flags |= O_CREAT;

			make_path(path_str.c_str());

			if (file_mode._truncate)
			{
				flags |= O_TRUNC;
			}
			else
			{
				flags |= O_APPEND;
			}
		}

		fd = open(path_str.c_str(), flags, 0666);
	}
};

struct g::io::file::itr::impl
{
	impl(const impl& f)
	{
		if (f->is_directory())
		{
/*
#if defined(__linux__) || defined(__APPLE__) 
			dir = opendir(f.file_impl->path.c_str());
#endif
			if (dir != nullptr)
			{
				operator++();
			}
*/
			auto dir = std::filesystem::path{f.file_impl.}
			_start
		}
	}

	~impl()
	{
		if (dir != nullptr)
		{
			closedir(dir);
		}
	}

	file& operator*()
	{
		return cur_file;
	}

	file* operator->()
	{
		return &cur_file;
	}

	itr& operator++();
	{
		if (dir != nullptr)
		{
			cur_ent = readdir(dir);
			cur_file = file(cur_ent->d_name);
		}

		return *this;
	}

	itr operator++(int)
	{
		itr temp = *this;
		operator++();
		return temp;
	}

	bool operator==(const itr& o)
	{
		if (dir == nullptr && o.dir == nullptr)
		{
			return true;
		}

		return cur_ent.d_ino == o.cur_ent.d_ino;
	}

	bool operator!=(const itr& o)
	{
		if ((cur_ent == nullptr) ^ (cur_ent == nullptr))
		{
			return true;
		}

		return cur_ent.d_ino != o.cur_ent.d_ino;
	}

protected:
#if defined(__linux__) || defined(__APPLE__)
	//DIR* dir;
	std::filesystem::directory_iterator _start, _end;
	file cur_file;
	struct dirent* cur_ent;
};

g::io::file::file(const std::string& path, const mode& mode)
{
	file_impl = std::make_unique<g::io::file::impl>(path.c_str(), mode);
}

g::io::file::file(const char* path, const mode& mode)
{
	file_impl = std::make_unique<g::io::file::impl>(path, mode);
}

g::io::file::~file() {}

g::io::file::file(file& o) noexcept : file_impl(o.file_impl) {}

g::io::file::file(file&& o) noexcept : file_impl(std::move(o.file_impl)) {}

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

std::vector<uint8_t> g::io::file::read_all()
{
	auto bytes = size();
	return read(bytes);
}	

int g::io::file::write(const void* buf, size_t bytes)
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

bool g::io::file::exists() const { return file_impl->exists(); }

bool g::io::file::is_directory() const { return file_impl->is_directory(); }

int g::io::file::get_fd() const { return file_impl->get_fd(); }

void g::io::file::make_path(const char* path) { g::io::file::impl::make_path(path); }

