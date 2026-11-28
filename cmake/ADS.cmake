# --------------------------------------------------------------------------- #
# ADS.cmake                                                                   #
#                                                                             #
# Fetches ADS (the header-only library of algorithms and data structures      #
# whose hash tables some headers of the core use) into the build tree, as     #
# smspp_fetch_ads(), after which ${ads_SOURCE_DIR}/include is the directory   #
# to add to the include paths. It is fetched once, even if more projects of   #
# the same build ask for it; FETCHCONTENT_SOURCE_DIR_ADS gives a source tree  #
# in place of the fetch, as a release tarball does [see the umbrella].        #
#                                                                             #
# The core fetches it and passes it on through the build interface of its     #
# target; an installed core does not carry it, so the package configuration   #
# file of the core includes this and gives it to the imported target, as the  #
# headers of the core include it.                                             #
# --------------------------------------------------------------------------- #

include(FetchContent)

macro(smspp_fetch_ads)
    FetchContent_Declare(
            ads
            GIT_REPOSITORY https://github.com/dmeoli/ADS.git
            GIT_TAG a3229dc32f55cafc8a1260c0d0842a6296c42230
            # only the headers are used: the sources are fetched, not built
            SOURCE_SUBDIR none)
    FetchContent_MakeAvailable(ads)
endmacro()
