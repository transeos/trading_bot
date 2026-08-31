// -*- C++ -*-
//
//*****************************************************************
//
// Cross-platform compatibility shims.
//
// This header lets the (originally Linux/GCC) sources build under MSVC on
// Windows without sprinkling #ifdefs across the codebase. On Windows it is
// force-included into every translation unit (see the root CMakeLists.txt,
// "/FI utils/PlatformCompat.h"); on Linux it simply forwards to the POSIX
// headers the code already relied on, so behaviour there is unchanged.
//
//*****************************************************************

#ifndef CRYPTOTRADER_PLATFORM_COMPAT_H
#define CRYPTOTRADER_PLATFORM_COMPAT_H

// Make M_PI and friends available from <cmath>/<math.h> under MSVC.
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

// libstdc++ pulls most of these in transitively, so GCC-targeted sources get
// away without including them explicitly. MSVC's STL does not, so include the
// common ones up-front to avoid a long tail of "identifier not found" errors.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#if defined(_WIN32)

// Pull in <windows.h> up-front (WIN32_LEAN_AND_MEAN and NOMINMAX are set
// globally by the build, so this stays minimal and skips the winsock1 and
// min/max macros). Doing it here, before any project header, means the handful
// of ALL-CAPS object-like macros it defines that collide with this codebase are
// neutralised exactly once. <windows.h> has include guards, so the transitive
// inclusions later (via asio / libcurl / cassandra) will not re-add them.
#include <windows.h>
#include <direct.h>  // _getcwd, _mkdir
#include <io.h>      // _access

// <winnt.h> defines DELETE as an object-like macro (0x00010000L). This codebase
// uses DELETE both as a rest_request_t enum value and as a function-like helper
// macro (see Globals.h); the Win32 access-right constant is not used anywhere.
#undef DELETE

// POSIX ssize_t
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef SSIZE_T ssize_t;
#endif

// POSIX getcwd() -> MSVC _getcwd()
#ifndef getcwd
#define getcwd _getcwd
#endif

// POSIX timegm() -> MSVC _mkgmtime()
#ifndef timegm
#define timegm _mkgmtime
#endif

// POSIX clock_gettime(): MSVC has no such function. Provide a wall-clock-only
// shim backed by the C11 timespec_get(). CLOCK_REALTIME is the only clock id
// the code uses.
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
inline int clock_gettime(int /*clk_id*/, struct timespec* ts) {
  return (timespec_get(ts, TIME_UTC) == TIME_UTC) ? 0 : -1;
}

#else  // POSIX

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#endif  // _WIN32

#endif  // CRYPTOTRADER_PLATFORM_COMPAT_H
