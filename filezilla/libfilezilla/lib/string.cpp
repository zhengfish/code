#include "libfilezilla/string.hpp"

#ifdef FZ_WINDOWS
#include <string.h>

#include "libfilezilla/private/windows.hpp"
#else
#include <iconv.h>
#include <strings.h>

#include <type_traits>
#endif

#include <cstdlib>

static_assert('a' + 25 == 'z', "We only support systems running with an ASCII-based character set. Sorry, no EBCDIC.");

// char may be unsigned, yielding stange results if subtracting characters. To work around it, expect a particular order of characters.
static_assert('A' < 'a', "We only support systems running with an ASCII-based character set. Sorry, no EBCDIC.");

namespace fz {

#ifdef FZ_WINDOWS
native_string to_native(std::string const& in)
{
	return to_wstring(in);
}

native_string to_native(std::wstring const& in)
{
	return in;
}
#else
native_string to_native(std::string const& in)
{
	return in;
}

native_string to_native(std::wstring const& in)
{
	return to_string(in);
}
#endif

int stricmp(std::string const& a, std::string const& b)
{
#ifdef FZ_WINDOWS
	return _stricmp(a.c_str(), b.c_str());
#else
	return strcasecmp(a.c_str(), b.c_str());
#endif
}

int stricmp(std::wstring const& a, std::wstring const& b)
{
#ifdef FZ_WINDOWS
	return _wcsicmp(a.c_str(), b.c_str());
#else
	return wcscasecmp(a.c_str(), b.c_str());
#endif
}

template<>
std::wstring::value_type tolower_ascii(std::wstring::value_type c)
{
	if (c >= 'A' && c <= 'Z') {
		return c + ('a' - 'A');
	}
	else if (c == 0x130 || c == 0x131) {
		c = 'i';
	}
	return c;
}

template<>
std::wstring::value_type toupper_ascii(std::wstring::value_type c)
{
	if (c >= 'a' && c <= 'z') {
		return c + ('A' - 'a');
	}
	else if (c == 0x130 || c == 0x131) {
		c = 'I';
	}
	return c;
}

std::wstring to_wstring(std::string const& in)
{
	std::mbstate_t ps{};
	std::wstring ret;

	char const* in_p = in.c_str();
	size_t len = std::mbsrtowcs(nullptr, &in_p, 0, &ps);
	if (len != static_cast<size_t>(-1)) {
		ret.resize(len);
		wchar_t* out_p = &ret[0];

		in_p = in.c_str(); // Some implementations of mbsrtowcs change src even on null dst
		std::mbsrtowcs(out_p, &in_p, len + 1, &ps);
	}

	return ret;
}

#ifndef FZ_WINDOWS
// On some platforms, e.g. NetBSD, the second argument to iconv is const.
// Depending which one is used, declare iconv_second_arg_type as either char* or char const*
extern "C" typedef size_t (*iconv_prototype_with_const)(iconv_t, char const**, size_t *, char**, size_t *);
typedef std::conditional<std::is_same<decltype(&iconv), iconv_prototype_with_const>::value, char const*, char*>::type iconv_second_arg_type;

namespace {
// On some platforms, e.g. those derived from SunOS, iconv does not understand "WCHAR_T", so we
// need to guess an encoding.
// On other platforms, WCHAR_T results in iconv() doing endless loops, such as OS X.
char const* calc_wchar_t_encoding()
{
	auto try_encoding = [](char const* const encoding) -> bool {
		iconv_t cd = iconv_open(encoding, "UTF-8");
		if (cd == reinterpret_cast<iconv_t>(-1)) {
			return false;
		}
		iconv_close(cd);
		return true;
	};

	// Explicitly specify endianess, otherwise we'll get a BOM prefixed to everything
	int const i = 1;
	char const* p = reinterpret_cast<char const*>(&i);
	bool const little_endian = p[0] == 1;

	if (sizeof(wchar_t) == 4) {
		if (little_endian && try_encoding("UTF-32LE")) {
			return "UTF-32LE";
		}
		if (!little_endian && try_encoding("UTF-32BE")) {
			return "UTF-32BE";
		}
	}
	else if (sizeof(wchar_t) == 2) {
		if (little_endian && try_encoding("UTF-16LE")) {
			return "UTF-16LE";
		}
		if (!little_endian && try_encoding("UTF-16BE")) {
			return "UTF-16BE";
		}
	}

	// Oh dear...
	// WCHAR_T is our last, best hope.
	return "WCHAR_T";
}

char const* wchar_t_encoding()
{
	static char const* const encoding = calc_wchar_t_encoding();
	return encoding;
}

class iconv_t_holder
{
public:
	iconv_t_holder(char const* to, char const* from)
	{
		cd = iconv_open(to, from);
	}

	~iconv_t_holder()
	{
		if (cd != reinterpret_cast<iconv_t>(-1)) {
			iconv_close(cd);
		}
	}

	explicit operator bool() const {
		return cd != reinterpret_cast<iconv_t>(-1);
	}

	iconv_t_holder(iconv_t_holder const&) = delete;
	iconv_t_holder& operator=(iconv_t_holder const&) = delete;

	iconv_t cd{reinterpret_cast<iconv_t>(-1)};
};
}
#endif

std::wstring to_wstring_from_utf8(std::string const& in)
{
	return to_wstring_from_utf8(in.c_str(), in.size());
}

std::wstring to_wstring_from_utf8(char const* s, size_t len)
{
	std::wstring ret;

	if (len != 0) {
#if FZ_WINDOWS
		char const* const in_p = s;
		int out_len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, in_p, static_cast<int>(len), nullptr, 0);
		if (out_len > 0) {
			ret.resize(out_len);
			wchar_t* out_p = &ret[0];
			MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, in_p, static_cast<int>(len), out_p, out_len);
		}
#else
		static thread_local iconv_t_holder holder(wchar_t_encoding(), "UTF-8");

		if (holder && iconv(holder.cd, nullptr, nullptr, nullptr, nullptr) != static_cast<size_t>(-1)) {
			auto in_p = const_cast<iconv_second_arg_type>(s);
			size_t out_len = len * sizeof(wchar_t) * 2;
			char* out_buf = new char[out_len];
			char* out_p = out_buf;

			size_t r = iconv(holder.cd, &in_p, &len, &out_p, &out_len);

			if (r != static_cast<size_t>(-1)) {
				ret.assign(reinterpret_cast<wchar_t*>(out_buf), reinterpret_cast<wchar_t*>(out_p));
			}

			// Our buffer should big enough as well, so we can ignore errors such as E2BIG.

			delete [] out_buf;
		}
#endif
	}

	return ret;
}

std::string to_string(std::wstring const& in)
{
	std::mbstate_t ps{};
	std::string ret;

	wchar_t const* in_p = in.c_str();
	size_t len = std::wcsrtombs(nullptr, &in_p, 0, &ps);
	if (len != static_cast<size_t>(-1)) {
		ret.resize(len);
		char* out_p = &ret[0];

		in_p = in.c_str(); // Some implementations of wcsrtombs change src even on null dst
		std::wcsrtombs(out_p, &in_p, len + 1, &ps);
	}

	return ret;
}

std::string FZ_PUBLIC_SYMBOL to_utf8(std::string const& in)
{
	return to_utf8(to_wstring(in));
}

std::string FZ_PUBLIC_SYMBOL to_utf8(std::wstring const& in)
{
	std::string ret;

	if (!in.empty()) {
#if FZ_WINDOWS
		wchar_t const* const in_p = in.c_str();
		int len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, in_p, static_cast<int>(in.size()), nullptr, 0, nullptr, nullptr);
		if (len > 0) {
			ret.resize(len);
			char* out_p = &ret[0];
			WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, in_p, static_cast<int>(in.size()), out_p, len, nullptr, nullptr);
		}
#else
		static thread_local iconv_t_holder holder("UTF-8", wchar_t_encoding());

		if (holder && iconv(holder.cd, nullptr, nullptr, nullptr, nullptr) != static_cast<size_t>(-1)) {
			auto in_p = reinterpret_cast<iconv_second_arg_type>(const_cast<wchar_t*>(in.c_str()));
			size_t in_len = in.size() * sizeof(wchar_t);

			size_t out_len = in.size() * 4;
			char* out_buf = new char[out_len];
			char* out_p = out_buf;

			size_t r = iconv(holder.cd, &in_p, &in_len, &out_p, &out_len);

			if (r != static_cast<size_t>(-1)) {
				ret.assign(out_buf, out_p);
			}

			// Our buffer should big enough as well, so we can ignore errors such as E2BIG.

			delete[] out_buf;
		}
#endif
	}

	return ret;
}

namespace {
template<typename String>
inline bool do_replace_substrings(String& in, String const& find, String const& replacement)
{
	bool ret = false;
	size_t pos = in.find(find);
	while (pos != std::string::npos) {
		in.replace(pos, find.size(), replacement);
		pos = in.find(find, pos + replacement.size());
		ret = true;
	}
	return ret;
}
}

std::string replaced_substrings(std::string const& in, std::string const& find, std::string const& replacement)
{
	std::string ret = in;
	do_replace_substrings(ret, find, replacement);
	return ret;
}

std::wstring replaced_substrings(std::wstring const& in, std::wstring const& find, std::wstring const& replacement)
{
	std::wstring ret = in;
	do_replace_substrings(ret, find, replacement);
	return ret;
}

bool replace_substrings(std::string& in, std::string const& find, std::string const& replacement)
{
	return do_replace_substrings(in, find, replacement);
}

bool replace_substrings(std::wstring& in, std::wstring const& find, std::wstring const& replacement)
{
	return do_replace_substrings(in, find, replacement);
}

}
