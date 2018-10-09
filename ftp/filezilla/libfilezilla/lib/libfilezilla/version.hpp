#ifndef LIBFILEZILLA_VERSION_HEADER
#define LIBFILEZILLA_VERSION_HEADER

/** \file
 * \brief Macros and functions to get the version of the headers and the library
 */

#include "libfilezilla.hpp"

#include <tuple>
/// \brief Version string of the libfilezilla headers
#define LIBFILEZILLA_VERSION "0.13.1"

#define LIBFILEZILLA_VERSION_MAJOR  0
#define LIBFILEZILLA_VERSION_MINOR  13
#define LIBFILEZILLA_VERSION_MICRO  1
#define LIBFILEZILLA_VERSION_NANO   0

/// \brief Suffix string, e.g. "rc1"
#define LIBFILEZILLA_VERSION_SUFFIX "0.13.1"

namespace fz {
/// \brief Get version string of libfilezilla
std::string FZ_PUBLIC_SYMBOL get_version_string();

/// \brief Get version of libfilezilla broken down into components major, minor, micro, nano and suffix
std::tuple<int, int, int, int, std::string> FZ_PUBLIC_SYMBOL get_version();
}
#endif
