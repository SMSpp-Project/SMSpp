# --------------------------------------------------------------------------- #
# SMSppPluginTable.cmake                                                      #
#                                                                             #
# Run with cmake -P after the link of a plugin [see SMSppModule.cmake]:       #
# writes the tables SMSpp_TABLES, separated by ?, of the plugin SMSpp_PLUGIN  #
# by the tool SMSpp_TOOL, which loads first the libraries SMSpp_LIBRARIES,    #
# separated by ?, with the directories of the DLLs SMSpp_DLLS, separated by   #
# ?, first in the PATH, so that the plugin finds them where they are not      #
# next to it. The tool runs with OpenBLAS on one thread, as the tests do: a   #
# plugin linked to it starts one thread per core when it is loaded, and under #
# a limit on the address space [ulimit -v] of a machine with many cores the   #
# buffers of those threads do not fit and the tool hangs.                     #
# --------------------------------------------------------------------------- #

string(REPLACE "?" ";" _libs "${SMSpp_LIBRARIES}")
string(REPLACE "?" ";" _dlls "${SMSpp_DLLS}")
foreach (_dll IN LISTS _dlls)
    get_filename_component(_dir "${_dll}" DIRECTORY)
    file(TO_NATIVE_PATH "${_dir}" _dir)
    set(ENV{PATH} "${_dir};$ENV{PATH}")
endforeach ()
set(ENV{OPENBLAS_NUM_THREADS} 1)
string(REPLACE "?" ";" _tables "${SMSpp_TABLES}")
foreach (_table IN LISTS _tables)
    get_filename_component(_dir "${_table}" DIRECTORY)
    file(MAKE_DIRECTORY "${_dir}")
    execute_process(COMMAND "${SMSpp_TOOL}" "${_table}" "${SMSpp_PLUGIN}"
            ${_libs} RESULT_VARIABLE _result)
    if (NOT _result EQUAL 0)
        message(FATAL_ERROR "the table ${_table} of ${SMSpp_PLUGIN} is not "
                "written")
    endif ()
endforeach ()
