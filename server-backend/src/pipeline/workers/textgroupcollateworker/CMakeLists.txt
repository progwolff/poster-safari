project(Postr)
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR) 

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../../../cmake/")

find_package(JsonCpp REQUIRED)
find_package(OpenCV REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(EasyLoggingpp REQUIRED)

include_directories("${EASYLOGGINGPP_INCLUDE_DIR}")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../../../common/")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../../../util/")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../")
include_directories("${CMAKE_CURRENT_LIST_DIR}/./")
include_directories(${JsonCpp_INCLUDE_DIR})

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
set(textgroupcollateworker_INCLUDES ${dirs} PARENT_SCOPE)

set(textgroupcollateworker_LIBS ${EASYLOGGINGPP_LIBRARY} ${JsonCpp_LIBRARY} ${OpenCV_LIBS} Threads::Threads uuid)
set(textgroupcollateworker_LIBS ${textgroupcollateworker_LIBS} PARENT_SCOPE)

option(BUILD_SHARED_WORKER_LIBS "Build shared libs" OFF)
option(BUILD_TextGroupCollateWORKER_EXECUTABLE "Build executable of TextGroupCollateWorker" OFF)

SET(textgroupcollateworker_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/imagedata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/postrdata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/base64.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/util.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../worker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../asyncworker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/textgroupcollateworker.cpp
    ${EASYLOGGINGPP_INCLUDE_DIR}/easylogging++.cc
)
SET(textgroupcollateworker_SRCS "${textgroupcollateworker_SRCS}" PARENT_SCOPE)

include(GNUInstallDirs)
if(NOT DEFINED CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
endif()
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_BINDIR "lib" CACHE PATH "user executables (bin)")
endif()

if(BUILD_SHARED_WORKER_LIBS)
    add_library(postr_textgroupcollateworker ${textgroupcollateworker_SRCS})
    target_compile_definitions(postr_textgroupcollateworker PRIVATE -DWORKER_LIBRARY=1) 
    target_link_libraries(postr_textgroupcollateworker ${textgroupcollateworker_LIBS})
    install(TARGETS postr_textgroupcollateworker DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_SHARED_WORKER_LIBS)

if(BUILD_TextGroupCollateWORKER_EXECUTABLE)
    add_executable(postr_textgroupcollate ${textgroupcollateworker_SRCS} main.cpp)
    target_link_libraries(postr_textgroupcollate ${textgroupcollateworker_LIBS})
    install(TARGETS postr_textgroupcollate DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(BUILD_TextGroupCollateWORKER_EXECUTABLE)
