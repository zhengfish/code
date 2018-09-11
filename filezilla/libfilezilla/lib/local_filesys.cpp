#include "libfilezilla/local_filesys.hpp"

#ifndef FZ_WINDOWS
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <utime.h>
#endif

namespace fz {

namespace {
template<typename T>
int64_t make_int64fzT(T hi, T lo)
{
	return (static_cast<int64_t>(hi) << 32) + static_cast<int64_t>(lo);
}
}

#ifdef FZ_WINDOWS
char const local_filesys::path_separator = '\\';
#else
char const local_filesys::path_separator = '/';
#endif


local_filesys::~local_filesys()
{
	end_find_files();
}

local_filesys::type local_filesys::get_file_type(native_string const& path, bool follow_links)
{
	if (path.size() > 1 && is_separator(path.back())) {
		native_string tmp = path;
		tmp.pop_back();
		return get_file_type(tmp);
	}

#ifdef FZ_WINDOWS
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(path.c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE) {
		return unknown;
	}
	FindClose(hFind);

	bool is_dir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

	if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT && IsReparseTagNameSurrogate(data.dwReserved0)) {
		if (!follow_links) {
			return link;
		}

		// Follow the reparse point
		HANDLE hFile = is_dir ? INVALID_HANDLE_VALUE : CreateFile(path.c_str(), FILE_READ_ATTRIBUTES | FILE_READ_EA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE) {
			return unknown;
		}

		BY_HANDLE_FILE_INFORMATION info{};
		int ret = GetFileInformationByHandle(hFile, &info);
		CloseHandle(hFile);
		if (!ret) {
			return unknown;
		}

		is_dir = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	return is_dir ? dir : file;
#else
	struct stat buf;
	int result = lstat(path.c_str(), &buf);
	if (result) {
		return unknown;
	}

#ifdef S_ISLNK
	if (S_ISLNK(buf.st_mode)) {
		if (!follow_links) {
			return link;
		}

		result = stat(path.c_str(), &buf);
		if (result) {
			return unknown;
		}
	}
#endif

	if (S_ISDIR(buf.st_mode)) {
		return dir;
	}

	return file;
#endif
}

local_filesys::type local_filesys::get_file_info(native_string const& path, bool &is_link, int64_t* size, datetime* modification_time, int *mode)
{
	if (path.size() > 1 && is_separator(path.back())) {
		native_string tmp = path;
		tmp.pop_back();
		return get_file_info(tmp, is_link, size, modification_time, mode);
	}
#ifdef FZ_WINDOWS
	is_link = false;

	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(path.c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE) {
		if (size) {
			*size = -1;
		}
		if (mode) {
			*mode = 0;
		}
		if (modification_time) {
			*modification_time = datetime();
		}
		return unknown;
	}
	FindClose(hFind);

	bool is_dir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

	if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT && IsReparseTagNameSurrogate(data.dwReserved0)) {
		is_link = true;

		HANDLE hFile = is_dir ? INVALID_HANDLE_VALUE : CreateFile(path.c_str(), FILE_READ_ATTRIBUTES | FILE_READ_EA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE) {
			BY_HANDLE_FILE_INFORMATION info{};
			int ret = GetFileInformationByHandle(hFile, &info);
			CloseHandle(hFile);
			if (ret != 0) {
				if (modification_time) {
					if (!modification_time->set(info.ftLastWriteTime, datetime::milliseconds)) {
						modification_time->set(info.ftCreationTime, datetime::milliseconds);
					}
				}

				if (mode) {
					*mode = (int)info.dwFileAttributes;
				}

				if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					if (size) {
						*size = -1;
					}
					return dir;
				}

				if (size) {
					*size = make_int64fzT(info.nFileSizeHigh, info.nFileSizeLow);
				}

				return file;
			}
		}

		if (size) {
			*size = -1;
		}
		if (mode) {
			*mode = 0;
		}
		if (modification_time) {
			*modification_time = datetime();
		}
		return is_dir ? dir : unknown;
	}

	if (modification_time) {
		*modification_time = datetime(data.ftLastWriteTime, datetime::milliseconds);
		if (modification_time->empty()) {
			*modification_time = datetime(data.ftCreationTime, datetime::milliseconds);
		}
	}

	if (mode) {
		*mode = (int)data.dwFileAttributes;
	}

	if (is_dir) {
		if (size) {
			*size = -1;
		}
		return dir;
	}
	else {
		if (size) {
			*size = make_int64fzT(data.nFileSizeHigh, data.nFileSizeLow);
		}
		return file;
	}
#else
	struct stat buf;
	static_assert(sizeof(buf.st_size) >= 8, "The st_size member of struct stat must be 8 bytes or larger.");

	int result = lstat(path.c_str(), &buf);
	if (result) {
		is_link = false;
		if (size) {
			*size = -1;
		}
		if (mode) {
			*mode = -1;
		}
		if (modification_time) {
			*modification_time = datetime();
		}
		return unknown;
	}

#ifdef S_ISLNK
	if (S_ISLNK(buf.st_mode)) {
		is_link = true;
		result = stat(path.c_str(), &buf);
		if (result) {
			if (size) {
				*size = -1;
			}
			if (mode) {
				*mode = -1;
			}
			if (modification_time) {
				*modification_time = datetime();
			}
			return unknown;
		}
	}
	else
#endif
		is_link = false;

	if (modification_time) {
		*modification_time = datetime(buf.st_mtime, datetime::seconds);
	}

	if (mode) {
		*mode = buf.st_mode & 0x777;
	}

	if (S_ISDIR(buf.st_mode)) {
		if (size) {
			*size = -1;
		}
		return dir;
	}

	if (size) {
		*size = buf.st_size;
	}

	return file;
#endif
}

bool local_filesys::begin_find_files(native_string path, bool dirs_only)
{
	if (path.empty()) {
		return false;
	}

	end_find_files();

	dirs_only_ = dirs_only;
#ifdef FZ_WINDOWS
	if (is_separator(path.back())) {
		m_find_path = path;
		path += '*';
	}
	else {
		m_find_path = path + fzT("\\");
		path += fzT("\\*");
	}

	m_hFind = FindFirstFileEx(path.c_str(), FindExInfoStandard, &m_find_data, dirs_only ? FindExSearchLimitToDirectories : FindExSearchNameMatch, nullptr, 0);
	if (m_hFind == INVALID_HANDLE_VALUE) {
		has_next_ = false;
		return false;
	}

	has_next_ = true;
	return true;
#else
	if (path.size() > 1 && path.back() == '/') {
		path.pop_back();
	}

	dir_ = opendir(path.c_str());
	if (!dir_) {
		return false;
	}

	m_raw_path = new char[path.size() + 2048 + 2];
	m_buffer_length = path.size() + 2048 + 2;
	strcpy(m_raw_path, path.c_str());
	if (path != fzT("/")) {
		m_raw_path[path.size()] = '/';
		m_file_part = m_raw_path + path.size() + 1;
	}
	else {
		m_file_part = m_raw_path + path.size();
	}

	return true;
#endif
}

void local_filesys::end_find_files()
{
#ifdef FZ_WINDOWS
	has_next_ = false;
	if (m_hFind != INVALID_HANDLE_VALUE) {
		FindClose(m_hFind);
		m_hFind = INVALID_HANDLE_VALUE;
	}
#else
	if (dir_) {
		closedir(dir_);
		dir_ = nullptr;
	}
	delete [] m_raw_path;
	m_raw_path = nullptr;
	m_file_part = nullptr;
#endif
}

bool local_filesys::get_next_file(native_string& name)
{
#ifdef FZ_WINDOWS
	if (!has_next_) {
		return false;
	}
	do {
		name = m_find_data.cFileName;
		if (name.empty()) {
			has_next_ = FindNextFile(m_hFind, &m_find_data) != 0;
			return true;
		}
		if (name == fzT(".") || name == fzT("..")) {
			continue;
		}

		if (dirs_only_ && !(m_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			continue;
		}

		has_next_ = FindNextFile(m_hFind, &m_find_data) != 0;
		return true;
	} while ((has_next_ = FindNextFile(m_hFind, &m_find_data) != 0));

	return false;
#else
	if (!dir_) {
		return false;
	}

	struct dirent* entry;
	while ((entry = readdir(dir_))) {
		if (!entry->d_name[0] ||
			!strcmp(entry->d_name, ".") ||
			!strcmp(entry->d_name, ".."))
			continue;

		if (dirs_only_) {
#if HAVE_STRUCT_DIRENT_D_TYPE
			if (entry->d_type == DT_LNK) {
				bool wasLink;
				alloc_path_buffer(entry->d_name);
				strcpy(m_file_part, entry->d_name);
				if (get_file_info(m_raw_path, wasLink, nullptr, nullptr, nullptr) != dir) {
					continue;
				}
			}
			else if (entry->d_type != DT_DIR) {
				continue;
			}
#else
			// Solaris doesn't have d_type
			bool wasLink;
			alloc_path_buffer(entry->d_name);
			strcpy(m_file_part, entry->d_name);
			if (get_file_info(m_raw_path, wasLink, nullptr, nullptr, nullptr) != dir) {
				continue;
			}
#endif
		}

		name = entry->d_name;

		return true;
	}

	return false;
#endif
}

bool local_filesys::get_next_file(native_string& name, bool &is_link, bool &is_dir, int64_t* size, datetime* modification_time, int* mode)
{
#ifdef FZ_WINDOWS
	if (!has_next_) {
		return false;
	}
	do {
		if (!m_find_data.cFileName[0]) {
			continue;
		}
		if (dirs_only_ && !(m_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			continue;
		}

		if (m_find_data.cFileName[0] == '.' && (!m_find_data.cFileName[1] || (m_find_data.cFileName[1] == '.' && !m_find_data.cFileName[2]))) {
			continue;
		}
		name = m_find_data.cFileName;

		is_dir = (m_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		is_link = (m_find_data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0 && IsReparseTagNameSurrogate(m_find_data.dwReserved0);
		if (is_link) {
			// Follow the reparse point
			HANDLE hFile = is_dir ? INVALID_HANDLE_VALUE : CreateFile((m_find_path + name).c_str(), FILE_READ_ATTRIBUTES | FILE_READ_EA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile != INVALID_HANDLE_VALUE) {
				BY_HANDLE_FILE_INFORMATION info{};
				int ret = GetFileInformationByHandle(hFile, &info);
				CloseHandle(hFile);
				if (ret != 0) {

					if (modification_time) {
						*modification_time = datetime(info.ftLastWriteTime, datetime::milliseconds);
						if (modification_time->empty()) {
							*modification_time = datetime(info.ftCreationTime, datetime::milliseconds);
						}
					}

					if (mode) {
						*mode = (int)info.dwFileAttributes;
					}

					is_dir = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
					if (size) {
						if (is_dir) {
							*size = -1;
						}
						else {
							*size = make_int64fzT(info.nFileSizeHigh, info.nFileSizeLow);
						}
					}

					has_next_ = FindNextFile(m_hFind, &m_find_data) != 0;
					return true;
				}
			}

			if (dirs_only_ && !is_dir) {
				continue;
			}

			if (size) {
				*size = -1;
			}
			if (mode) {
				*mode = 0;
			}
			if (modification_time) {
				*modification_time = datetime();
			}
		}
		else {
			if (modification_time) {
				*modification_time = datetime(m_find_data.ftLastWriteTime, datetime::milliseconds);
				if (modification_time->empty()) {
					*modification_time = datetime(m_find_data.ftLastWriteTime, datetime::milliseconds);
				}
			}

			if (mode) {
				*mode = (int)m_find_data.dwFileAttributes;
			}

			if (size) {
				if (is_dir) {
					*size = -1;
				}
				else {
					*size = make_int64fzT(m_find_data.nFileSizeHigh, m_find_data.nFileSizeLow);
				}
			}
		}
		has_next_ = FindNextFile(m_hFind, &m_find_data) != 0;
		return true;
	} while ((has_next_ = FindNextFile(m_hFind, &m_find_data) != 0));

	return false;
#else
	if (!dir_) {
		return false;
	}

	struct dirent* entry;
	while ((entry = readdir(dir_))) {
		if (!entry->d_name[0] ||
			!strcmp(entry->d_name, ".") ||
			!strcmp(entry->d_name, ".."))
			continue;

#if HAVE_STRUCT_DIRENT_D_TYPE
		if (dirs_only_) {
			if (entry->d_type == DT_LNK) {
				alloc_path_buffer(entry->d_name);
				strcpy(m_file_part, entry->d_name);
				type t = get_file_info(m_raw_path, is_link, size, modification_time, mode);
				if (t != dir) {
					continue;
				}

				name = entry->d_name;
				is_dir = true;
				return true;
			}
			else if (entry->d_type != DT_DIR) {
				continue;
			}
		}
#endif

		alloc_path_buffer(entry->d_name);
		strcpy(m_file_part, entry->d_name);
		type t = get_file_info(m_raw_path, is_link, size, modification_time, mode);

		if (t == unknown) { // Happens for example in case of permission denied
#if HAVE_STRUCT_DIRENT_D_TYPE
			t = (entry->d_type == DT_DIR) ? dir : file;
#else
			t = file;
#endif
			is_link = false;
			if (size) {
				*size = -1;
			}
			if (modification_time) {
				*modification_time = datetime();
			}
			if (mode) {
				*mode = 0;
			}
		}
		if (dirs_only_ && t != dir) {
			continue;
		}

		is_dir = t == dir;

		name = entry->d_name;

		return true;
	}

	return false;
#endif
}

#ifndef FZ_WINDOWS
void local_filesys::alloc_path_buffer(char const* filename)
{
	int len = strlen(filename);
	int pathlen = m_file_part - m_raw_path;

	if (len + pathlen >= m_buffer_length) {
		m_buffer_length = (len + pathlen) * 2;
		char* tmp = new char[m_buffer_length];
		memcpy(tmp, m_raw_path, pathlen);
		delete [] m_raw_path;
		m_raw_path = tmp;
		m_file_part = m_raw_path + pathlen;
	}
}
#endif

datetime local_filesys::get_modification_time(native_string const& path)
{
	datetime mtime;

	bool tmp;
	if (get_file_info(path, tmp, nullptr, &mtime, nullptr) == unknown) {
		mtime = datetime();
	}

	return mtime;
}

bool local_filesys::set_modification_time(native_string const& path, datetime const& t)
{
	if (t.empty()) {
		return false;
	}

#ifdef FZ_WINDOWS
	FILETIME ft = t.get_filetime();
	if (!ft.dwHighDateTime) {
		return false;
	}

	HANDLE h = CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (h == INVALID_HANDLE_VALUE) {
		return false;
	}

	bool ret = SetFileTime(h, nullptr, &ft, &ft) == TRUE;
	CloseHandle(h);
	return ret;
#else
	utimbuf utm{};
	utm.actime = t.get_time_t();
	utm.modtime = utm.actime;
	return utime(path.c_str(), &utm) == 0;
#endif
}

int64_t local_filesys::get_size(native_string const& path, bool* is_link)
{
	int64_t ret = -1;
	bool tmp{};
	type t = get_file_info(path, is_link ? *is_link : tmp, &ret, nullptr, nullptr);
	if (t != file) {
		ret = -1;
	}

	return ret;
}

native_string local_filesys::get_link_target(native_string const& path)
{
	native_string target;

#ifdef FZ_WINDOWS
	HANDLE hFile = CreateFile(path.c_str(), FILE_READ_ATTRIBUTES | FILE_READ_EA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD const size = 1024;
		native_string::value_type out[size];
		DWORD ret = GetFinalPathNameByHandle(hFile, out, size, 0);
		if (ret > 0 && ret < size) {
			target = out;
		}
		CloseHandle(hFile);
	}
#else
	size_t const size = 1024;
	char out[size];

	ssize_t res = readlink(path.c_str(), out, size);
	if (res > 0 && static_cast<size_t>(res) < size) {
		out[res] = 0;
		target = out;
	}
#endif
	return target;
}

}
