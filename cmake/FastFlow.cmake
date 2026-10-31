# --------------------------------------------------------------------------- #
# FastFlow.cmake                                                              #
#                                                                             #
# Fetches FastFlow (the header-only library of parallel patterns that some    #
# sources of the modules use) into the build tree, as smspp_fetch_fastflow(), #
# after which fastflow_SOURCE_DIR is the directory to add to the include      #
# paths. It is fetched once, even if more projects of the same build ask for  #
# it; FETCHCONTENT_SOURCE_DIR_FASTFLOW gives a source tree in place of the    #
# fetch, as a release tarball does [see the umbrella].                        #
#                                                                             #
# The core fetches it and passes it on through the build interface of its     #
# target; an installed core does not carry it, so a module built alone with   #
# find_package() fetches it with this, which the package configuration file   #
# of the core includes:                                                       #
#     smspp_fetch_fastflow()                                                  #
#     target_include_directories(<target> PRIVATE ${fastflow_SOURCE_DIR})     #
# --------------------------------------------------------------------------- #

include(FetchContent)

macro(smspp_fetch_fastflow)
    FetchContent_Declare(
            fastflow
            GIT_REPOSITORY https://github.com/fastflow/fastflow.git
            GIT_TAG master)

    FetchContent_GetProperties(fastflow)
    if (NOT fastflow_POPULATED)
        FetchContent_Populate(fastflow)
    endif ()

    # the MSVC compatibility patch: the Windows port of FastFlow
    # (ff/platforms/pthread_minport_windows.h) is incomplete under MSVC, as
    # it provides pthread_cond_wait() but not pthread_cond_timedwait(), nor
    # clock_gettime() and CLOCK_REALTIME, which the core of FastFlow
    # (ff/node.hpp, lb.hpp, gt.hpp, farm.hpp, pipeline.hpp and ff/utils.hpp)
    # uses for its timed waits in blocking mode; on Linux these come from
    # <pthread.h> and <time.h>, on Windows a minimal implementation is put in
    # the fetched header at configure time. The edit is idempotent (guarded
    # by the FF_SMSPP_WIN_COMPAT marker), as the header shim of LEMON in
    # MCFLemonSolver/CMakeLists.txt; NOMINMAX and /Zc:__cplusplus (set in
    # the CMakeSettings.txt of the umbrella) are also needed to build
    # FastFlow with MSVC.
    if (MSVC)
        set(_ff_winport "${fastflow_SOURCE_DIR}/ff/platforms")
        string(APPEND _ff_winport "/pthread_minport_windows.h")
        if (EXISTS "${_ff_winport}")
            file(READ "${_ff_winport}" _ff_winport_src)
            if (NOT _ff_winport_src MATCHES "FF_SMSPP_WIN_COMPAT")
                # put right after "typedef CONDITION_VARIABLE pthread_cond_t;",
                # where INLINE, RESTRICT, <time.h> (struct timespec),
                # <errno.h> (ETIMEDOUT) and <Windows.h> are in scope;
                # pthread_mutex_t is CRITICAL_SECTION and pthread_cond_t is
                # CONDITION_VARIABLE there
                set(_ff_shim [=[typedef CONDITION_VARIABLE pthread_cond_t;
/* ---- FF_SMSPP_WIN_COMPAT: MSVC compatibility shim injected by SMS++ ---- */
#ifndef FF_SMSPP_WIN_COMPAT
#define FF_SMSPP_WIN_COMPAT
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
/* clock_gettime(): map CLOCK_REALTIME onto the Windows system clock. FILETIME
   counts 100 ns ticks since 1601-01-01; 116444736000000000 is the offset (in
   ticks) from that epoch to the Unix epoch 1970-01-01. */
INLINE int clock_gettime(int, struct timespec *tp) {
    FILETIME ft; ULARGE_INTEGER u;
    GetSystemTimePreciseAsFileTime(&ft);
    u.LowPart = ft.dwLowDateTime; u.HighPart = ft.dwHighDateTime;
    unsigned long long t = u.QuadPart - 116444736000000000ULL;
    tp->tv_sec  = (long)(t / 10000000ULL);
    tp->tv_nsec = (long)((t % 10000000ULL) * 100ULL);
    return 0;
}
/* pthread_cond_timedwait(): turn the absolute POSIX deadline into a relative
   millisecond timeout for SleepConditionVariableCS(); return ETIMEDOUT on
   expiry, as POSIX requires. */
INLINE int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                  const struct timespec *abstime) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    long long ms = (long long)(abstime->tv_sec - now.tv_sec) * 1000LL
                 + (long long)(abstime->tv_nsec - now.tv_nsec) / 1000000LL;
    if (ms < 0) ms = 0;
    if (!SleepConditionVariableCS(cond, mutex, (DWORD)ms))
        if (GetLastError() == ERROR_TIMEOUT) return ETIMEDOUT;
    return 0;
}
#endif
/* ---- end FF_SMSPP_WIN_COMPAT ---- */]=])
                string(REPLACE "typedef CONDITION_VARIABLE pthread_cond_t;"
                        "${_ff_shim}" _ff_winport_src "${_ff_winport_src}")
                file(WRITE "${_ff_winport}" "${_ff_winport_src}")
                message(STATUS "FastFlow: injected the MSVC shim of \
pthread_cond_timedwait and clock_gettime")
            endif ()
        endif ()
    endif ()

    # the compilers of MinGW (MSYS2) have the pthreads and the clocks of
    # POSIX, from winpthreads, while FastFlow takes any _WIN32 for MSVC and
    # its minimal port of the pthreads, whose types clash with those of
    # winpthreads: the conditions on _WIN32 of its headers leave MinGW out,
    # so that it goes the way of the POSIX systems (those that also ask for
    # _MSC_VER are left as they are). The edit is idempotent, guarded by the
    # FF_SMSPP_MINGW marker put in ff/platforms/platform.h
    if (MINGW)
        set(_ff_platform "${fastflow_SOURCE_DIR}/ff/platforms/platform.h")
        file(READ "${_ff_platform}" _ff_platform_src)
        if (NOT _ff_platform_src MATCHES "FF_SMSPP_MINGW")
            file(GLOB_RECURSE _ff_headers
                    "${fastflow_SOURCE_DIR}/ff/*.h"
                    "${fastflow_SOURCE_DIR}/ff/*.hpp")
            foreach (_ff_h IN LISTS _ff_headers)
                file(READ "${_ff_h}" _ff_src)
                set(_ff_new "${_ff_src}")
                string(REPLACE "defined(_WIN32)"
                        "(defined(_WIN32) && !defined(__MINGW32__))"
                        _ff_new "${_ff_new}")
                string(REPLACE "#ifdef _WIN32"
                        "#if defined(_WIN32) && !defined(__MINGW32__)"
                        _ff_new "${_ff_new}")
                if (NOT _ff_new STREQUAL _ff_src)
                    file(WRITE "${_ff_h}" "${_ff_new}")
                endif ()
            endforeach ()
            file(READ "${_ff_platform}" _ff_platform_src)
            file(WRITE "${_ff_platform}"
                    "/* FF_SMSPP_MINGW: _WIN32 without MinGW */\n"
                    "${_ff_platform_src}")
            message(STATUS "FastFlow: the conditions on _WIN32 leave MinGW \
out")
        endif ()
    endif ()
endmacro()
