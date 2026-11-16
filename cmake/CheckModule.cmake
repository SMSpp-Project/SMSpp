# --------------------------------------------------------------------------- #
# CheckModule.cmake                                                           #
#                                                                             #
# Decides whether the C++20 modules of SMS++ are built, and checks whether    #
# the one of an SMS++ module can be. SMSpp_BUILD_MODULE is                    #
#   - ON or OFF, if it is given (e.g., -DSMSpp_BUILD_MODULE=ON);              #
#   - otherwise ON where the toolchain builds the modules, and OFF elsewhere. #
# Where a module cannot be built, smspp_check_module() turns                  #
# SMSpp_BUILD_MODULE off in the calling directory, and the build goes on      #
# without it, using the headers as when the option is OFF, instead of         #
# failing halfway: with a warning that says why if the module was asked for,  #
# with a status message if it was not. The module needs                       #
#   - CMake 3.28 or later;                                                    #
#   - a generator and a compiler that build a module and a unit that          #
#     imports it and uses its templates, which a small module and a unit      #
#     built at configure time tell [see module-check]: e.g., Ninja 1.11       #
#     with Clang 16 and its clang-scan-deps, or MSVC 19.34, while GCC, up     #
#     to 15 at least, builds the module but not the units that import it;     #
#   - the modules it imports, i.e., the targets given as arguments (e.g.,     #
#     SMS++::SMS++_module), which a dependency that was installed without     #
#     its module does not have; they are looked for only when the module is   #
#     the top-level project, i.e., when it finds them with find_package(),    #
#     since in the umbrella they are defined by the modules that come later,  #
#     which pass the same checks.                                             #
# The same reason is reported with a warning only once, so that an umbrella   #
# build with all the modules does not repeat it for each of them.             #
#                                                                             #
# The umbrella and the core include it, and so does the package               #
# configuration file of the core, so that a module finds it both inside the   #
# umbrella and with find_package():                                           #
#     smspp_check_module(${modNamespace}::SMS++_module)                       #
#     if (SMSpp_BUILD_MODULE)                                                 #
#         ...                                                                 #
# --------------------------------------------------------------------------- #

# sets out to why the toolchain cannot build the modules, or to "" if it can:
# a small module and a unit that imports it are built to tell [see
# module-check], once per configuration, whose result the cache keeps
function(_smspp_module_toolchain out)
    set(_why "")
    if (CMAKE_VERSION VERSION_LESS 3.28)
        set(_why "CMake ${CMAKE_VERSION} is older than 3.28")
    else ()
        set(_scan "${CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS}")
        try_compile(SMSpp_MODULE_TOOLCHAIN PROJECT smspp_module_check
                SOURCE_DIR ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/module-check
                BINARY_DIR ${CMAKE_BINARY_DIR}/smspp-module-check
                CMAKE_FLAGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                    -DCMAKE_CXX_COMPILER_CLANG_SCAN_DEPS=${_scan})
        if (NOT SMSpp_MODULE_TOOLCHAIN)
            set(_why "the generator ${CMAKE_GENERATOR} and the compiler \
${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} do not build a module \
whose templates a unit that imports it can use [see the module-check of \
SMS++/cmake]")
        endif ()
    endif ()
    set(${out} "${_why}" PARENT_SCOPE)
endfunction()

# the default: ON where the toolchain builds the modules
if ((NOT DEFINED SMSpp_BUILD_MODULE) OR ("${SMSpp_BUILD_MODULE}" STREQUAL ""))
    set(SMSpp_BUILD_MODULE_AUTO TRUE)
    _smspp_module_toolchain(_smspp_why)
    if (NOT _smspp_why)
        set(SMSpp_BUILD_MODULE ON)
    else ()
        set(SMSpp_BUILD_MODULE OFF)
    endif ()
    unset(_smspp_why)
endif ()

function(smspp_check_module)
    if (NOT SMSpp_BUILD_MODULE)
        return()
    endif ()

    _smspp_module_toolchain(_why)

    if (NOT _why AND PROJECT_IS_TOP_LEVEL)
        foreach (_target IN LISTS ARGN)
            if (NOT TARGET ${_target})
                set(_why "${_target} is not there")
                break()
            endif ()
        endforeach ()
    endif ()

    if (_why)
        get_property(_said GLOBAL PROPERTY SMSpp_MODULE_OFF_REASONS)
        if (SMSpp_BUILD_MODULE_AUTO)
            message(STATUS "SMSpp_BUILD_MODULE is OFF for ${PROJECT_NAME}: \
${_why}")
        elseif (_why IN_LIST _said)
            message(STATUS "SMSpp_BUILD_MODULE is OFF for ${PROJECT_NAME}")
        else ()
            message(WARNING "SMSpp_BUILD_MODULE is turned OFF for \
${PROJECT_NAME}: ${_why}")
            set_property(GLOBAL APPEND PROPERTY SMSpp_MODULE_OFF_REASONS
                    "${_why}")
        endif ()
        set(SMSpp_BUILD_MODULE OFF PARENT_SCOPE)
    endif ()
endfunction()
