project(Postr)
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR) 

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../../../cmake/")

find_package(JsonCpp REQUIRED)
find_package(OpenCV REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(EasyLoggingpp REQUIRED)

find_package(Sparsehash REQUIRED)

include_directories("${EASYLOGGINGPP_INCLUDE_DIR}")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../../../common/")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../../../util/")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../")
include_directories("${CMAKE_CURRENT_LIST_DIR}/./")
include_directories(${JsonCpp_INCLUDE_DIR})
include_directories(${SPARSEHASH_INCLUDE_DIR})

message("${SPARSEHASH_INCLUDE_DIR}")

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
set(spellcorrectworker_INCLUDES ${dirs} PARENT_SCOPE)

set(spellcorrectworker_LIBS ${EASYLOGGINGPP_LIBRARY} ${JsonCpp_LIBRARY} ${OpenCV_LIBS} Threads::Threads uuid)
set(spellcorrectworker_LIBS ${spellcorrectworker_LIBS} PARENT_SCOPE)

option(BUILD_SHARED_WORKER_LIBS "Build shared libs" OFF)
option(BUILD_BGSEGMENTWORKER_EXECUTABLE "Build executable of BackgroundSegmentationWorker" OFF)

SET(spellcorrectworker_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/imagedata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/postrdata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/base64.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/util.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../worker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../asyncworker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/spellcorrectworker.cpp
    ${EASYLOGGINGPP_INCLUDE_DIR}/easylogging++.cc
)
SET(spellcorrectworker_SRCS "${spellcorrectworker_SRCS}" PARENT_SCOPE)

include(GNUInstallDirs)
if(NOT DEFINED CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
endif()
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_BINDIR "lib" CACHE PATH "user executables (bin)")
endif()

if(BUILD_SHARED_WORKER_LIBS)
    add_library(postr_spellcorrectworker ${spellcorrectworker_SRCS})
    target_compile_definitions(postr_spellcorrectworker PUBLIC -DWORKER_LIBRARY=1) 
    target_link_libraries(postr_spellcorrectworker ${spellcorrectworker_LIBS})
    install(TARGETS postr_spellcorrectworker DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_SHARED_WORKER_LIBS)

if(BUILD_BGSEGMENTWORKER_EXECUTABLE)
    add_executable(postr_spellcorrect ${spellcorrectworker_SRCS} main.cpp)
    target_link_libraries(postr_spellcorrect ${spellcorrectworker_LIBS})
    install(TARGETS postr_spellcorrect DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(BUILD_BGSEGMENTWORKER_EXECUTABLE)

install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/locations" DESTINATION ${CMAKE_INSTALL_DATADIR}/postr/spellcorrect)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/artists" DESTINATION ${CMAKE_INSTALL_DATADIR}/postr/spellcorrect)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/cities.txt" DESTINATION ${CMAKE_INSTALL_DATADIR}/postr/spellcorrect)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/dayofweek.txt" DESTINATION ${CMAKE_INSTALL_DATADIR}/postr/spellcorrect)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/substitutioncosts.txt" DESTINATION ${CMAKE_INSTALL_DATADIR}/postr/spellcorrect)
