# curlppConfig.cmake - Configuration file for external projects.
set(CURLPP_PATHS ${CURLPP_ROOT} $ENV{CURLPP_ROOT})

find_path(CURLPP_INCLUDE_DIR
        curlpp/cURLpp.hpp
        PATH_SUFFIXES include
        PATHS ${CURLPP_PATHS}
)

if (CURLPP_USE_STATIC_LIBS)
    message ("-- curlpp: Static linking is preferred")
    find_library(CURLPP_LIBRARY
        NAMES libcurlpp.a libcurlpp.dylib libcurlpp
        HINTS "${CMAKE_PREFIX_PATH}/lib"
    )
elseif (CURLPP_USE_SHARED_LIBS)
    message ("-- curlpp: Dynamic linking is preferred")
    find_library(CURLPP_LIBRARY
        NAMES libcurlpp.dylib libcurlpp libcurlpp.a
        HINTS "${CMAKE_PREFIX_PATH}/lib"
    )
endif()

find_package(CURL REQUIRED)

list(APPEND CURLPP_LIBRARIES ${CURLPP_LIBRARY})
list(APPEND CURLPP_LIBRARIES ${CURL_LIBRARIES})
list(APPEND CURLPP_LIBRARIES curlpp)

find_package_handle_standard_args(CURLPP REQUIRED_VARS CURLPP_INCLUDE_DIR)
