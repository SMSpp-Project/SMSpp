# --------------------------------------------------------------------------- #
# CheckModule.cmake                                                           #
#                                                                             #
# Decides whether the C++20 modules of SMS++ are built, and checks whether    #
# the one of an SMS++ module can be. SMSpp_BUILD_MODULE is                    #
#   - ON or OFF, if it is given (e.g., -DSMSpp_BUILD_MODULE=ON);              #
#   - otherwise ON where the toolchain builds the modules and the compiler    #
#     is Clang, and OFF elsewhere, MSVC included, where the modules are       #
#     built only on request.                                                  #
# Where a module cannot be built, smspp_check_module() turns                  #
# SMSpp_BUILD_MODULE off in the calling directory, and the build goes on      #
# without it, using the headers as when the option is OFF, instead of         #
# failing halfway: with a warning that says why if the module was asked for,  #
# with a status message if it was not. The module needs                       #
#   - CMake 3.28 or later;                                                    #
#   - the Ninja generator (Ninja 1.11 or later) or a Visual Studio one;       #
#   - a compiler whose module dependencies CMake can scan and that accepts    #
#     the headers of Eigen and Boost in a module: Clang 16 or MSVC 19.34 or   #
#     later, and with Clang also clang-scan-deps (GCC, up to 15 at least,     #
#     refuses the entities of internal linkage that those headers expose to   #
#     the templates of a module);                                             #
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

# sets out to why the toolchain cannot build the modules, or to "" if it can
function(_smspp_module_toolchain out)
    set(_why "")
    if (CMAKE_VERSION VERSION_LESS 3.28)
        set(_why "CMake ${CMAKE_VERSION} is older than 3.28")
    elseif (CMAKE_GENERATOR MATCHES "^Ninja")
        execute_process(COMMAND "${CMAKE_MAKE_PROGRAM}" --version
                OUTPUT_VARIABLE _ninja OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE _res)
        if (NOT _res EQUAL 0 OR _ninja VERSION_LESS 1.11)
            set(_why "Ninja ${_ninja} is older than 1.11")
        endif ()
    elseif (NOT CMAKE_GENERATOR MATCHES "^Visual Studio")
        set(_why "the generator ${CMAKE_GENERATOR} does not build modules \
(Ninja or Visual Studio is needed)")
    endif ()

    if (NOT _why)
        set(_id "${CMAKE_CXX_COMPILER_ID}")
        set(_ver "${CMAKE_CXX_COMPILER_VERSION}")
        if (_id STREQUAL "GNU")
            set(_why "GCC refuses the entities of internal linkage (e.g., \
Eigen::Dynamic) that the headers of Eigen and Boost expose to the templates \
of a module (Clang 16 or MSVC 19.34 is needed)")
        elseif (NOT ((_id STREQUAL "Clang" AND _ver VERSION_GREATER_EQUAL 16)
                     OR (_id STREQUAL "MSVC"
                         AND _ver VERSION_GREATER_EQUAL 19.34)))
            set(_why "the compiler ${_id} ${_ver} does not scan the module \
dependencies (Clang 16 or MSVC 19.34 is needed)")
        elseif (_id STREQUAL "Clang"
                AND NOT CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS)
            set(_why "clang-scan-deps, which scans the module dependencies \
for Clang, is not there (e.g., it is in clang-tools on Debian and Ubuntu)")
        endif ()
    endif ()
    set(${out} "${_why}" PARENT_SCOPE)
endfunction()

# the default: ON with Clang, where the toolchain builds the modules
if ((NOT DEFINED SMSpp_BUILD_MODULE) OR ("${SMSpp_BUILD_MODULE}" STREQUAL ""))
    set(SMSpp_BUILD_MODULE_AUTO TRUE)
    _smspp_module_toolchain(_smspp_why)
    if ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") AND (NOT _smspp_why))
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
