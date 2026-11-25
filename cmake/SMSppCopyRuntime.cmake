# --------------------------------------------------------------------------- #
# SMSppCopyRuntime.cmake                                                      #
#                                                                             #
# Run with cmake -P by smspp_copy_runtime() [see SMSppModule.cmake]: copies   #
# the files SMSpp_FILES, separated by ? (which no path of Windows has, and    #
# which cmd.exe does not take as a pipe, as it does |), into the directory    #
# SMSpp_DESTINATION, the ones already there with the same time stamp          #
# excepted, holding a lock on the directory so that two targets that share    #
# it do not copy the same file at the same time.                              #
# --------------------------------------------------------------------------- #

string(REPLACE "?" ";" _files "${SMSpp_FILES}")
if (_files)
    file(LOCK "${SMSpp_DESTINATION}" DIRECTORY GUARD PROCESS TIMEOUT 600)
    file(COPY ${_files} DESTINATION "${SMSpp_DESTINATION}")
endif ()
