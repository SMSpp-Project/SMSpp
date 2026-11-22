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
            GIT_TAG master
            # no CMakeLists.txt there: the sources are fetched, not built
            SOURCE_SUBDIR none)
    FetchContent_MakeAvailable(fastflow)

    # the MSVC compatibility patch: the Windows port of FastFlow
    # (ff/platforms/pthread_minport_windows.h) is incomplete under MSVC, as
    # it provides pthread_cond_wait() but not pthread_cond_timedwait(), nor
    # clock_gettime() and CLOCK_REALTIME, which the core of FastFlow
    # (ff/node.hpp, lb.hpp, gt.hpp, farm.hpp, pipeline.hpp and ff/utils.hpp)
    # uses for its timed waits in blocking mode; on Linux these come from
    # <pthread.h> and <time.h>, while a minimal implementation is put in the
    # fetched header at configure time, which only a build for Windows
    # includes. The edit is idempotent (guarded by the FF_SMSPP_WIN_COMPAT
    # marker), as the header shim of LEMON in
    # MCFLemonSolver/CMakeLists.txt; NOMINMAX and /Zc:__cplusplus (set in
    # the CMakeSettings.txt of the umbrella) are also needed to build
    # FastFlow with MSVC.
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

    # the MinGW patch: FastFlow takes Windows for MSVC, and MinGW-w64, which
    # has the POSIX threads of winpthreads (that its libstdc++ includes),
    # <sys/time.h> and <unistd.h>, clashes with the MSVC port in
    # ff/platforms/platform.h, and gets no alignment macros nor atom_t from
    # ff/platforms/liblfds.h. Under MinGW platform.h takes a branch of its own
    # as that of Linux, with the aligned allocation of Windows, and liblfds.h
    # the macros of GCC with a 64-bit atom_t; guarded by FF_SMSPP_MINGW
    set(_ff_platform "${fastflow_SOURCE_DIR}/ff/platforms/platform.h")
    set(_ff_lfds "${fastflow_SOURCE_DIR}/ff/platforms/liblfds.h")
    if (EXISTS "${_ff_platform}" AND EXISTS "${_ff_lfds}")
        file(READ "${_ff_platform}" _ff_src)
        if (NOT _ff_src MATCHES "FF_SMSPP_MINGW")
            set(_ff_branch [=[#elif defined(__MINGW32__)
/* ---- FF_SMSPP_MINGW: MinGW-w64 as Linux, by SMS++ ---- */
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <malloc.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
inline static int ff_smspp_posix_memalign(void **memptr, size_t alignment,
                                          size_t size)
{
    *memptr = _aligned_malloc(size, alignment);
    return( *memptr ? 0 : ENOMEM );
}
#define posix_memalign ff_smspp_posix_memalign
inline static void posix_memalign_free(void* mem)
{
    _aligned_free(mem);
}
/* ---- end FF_SMSPP_MINGW ---- */
#elif defined(__GNUC__) && (defined(__linux__) || defined(__APPLE__))]=])
            string(REPLACE "#if defined(_WIN32)"
                "#if defined(_WIN32) && !defined(__MINGW32__)"
                _ff_src "${_ff_src}")
            set(_ff_linux "#elif defined(__GNUC__) && ")
            string(APPEND _ff_linux
                "(defined(__linux__) || defined(__APPLE__))")
            string(REPLACE "${_ff_linux}" "${_ff_branch}" _ff_src "${_ff_src}")
            file(WRITE "${_ff_platform}" "${_ff_src}")

            file(READ "${_ff_lfds}" _ff_src)
            set(_ff_macros [=[/***** defines *****/
/* ---- FF_SMSPP_MINGW: the macros of GCC under MinGW-w64, by SMS++ ---- */
#if (defined __MINGW32__)
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define ALIGN_TO_PRE(alignment)
#define ALIGN_TO_POST(alignment)        __attribute__( (aligned(alignment)) )
#define FF_MEM_ALIGN(__A,__alignment) __A __attribute__((aligned(__alignment)))
#if (defined __x86_64__ || defined __aarch64__)
typedef unsigned long long int  atom_t;
#define ALIGN_SINGLE_POINTER    8
#define ALIGN_DOUBLE_POINTER    16
#else
typedef unsigned long int     atom_t;
#define ALIGN_SINGLE_POINTER  4
#define ALIGN_DOUBLE_POINTER  8
#endif
#endif
/* ---- end FF_SMSPP_MINGW ---- */]=])
            string(REPLACE "/***** defines *****/" "${_ff_macros}"
                _ff_src "${_ff_src}")
            string(REPLACE "#elif (defined _MSC_VER)"
                "#elif (defined _MSC_VER) || (defined __MINGW32__)"
                _ff_src "${_ff_src}")
            file(WRITE "${_ff_lfds}" "${_ff_src}")
            message(STATUS "FastFlow: injected the MinGW branch of the \
platform")
        endif ()
    endif ()
    # the CLH lock of ff/spin-lock.hpp swaps its pointers as long, which is of
    # 32 bits on Windows: under MinGW it swaps them as pointers, as under MSVC
    set(_ff_spin "${fastflow_SOURCE_DIR}/ff/spin-lock.hpp")
    set(_ff_clh "#if defined(_MSC_VER) // To be tested")
    if (EXISTS "${_ff_spin}")
        file(READ "${_ff_spin}" _ff_src)
        if (_ff_src MATCHES "#if defined\\(_MSC_VER\\) // To be tested")
            string(REPLACE "${_ff_clh}"
                "#if defined(_MSC_VER) || defined(__MINGW32__) // To be tested"
                _ff_src "${_ff_src}")
            file(WRITE "${_ff_spin}" "${_ff_src}")
        endif ()
    endif ()
endmacro()
