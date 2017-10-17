project(Postr)
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR) 

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../../../cmake/")

find_package(JsonCpp REQUIRED)
find_package(OpenCV REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(EasyLoggingpp REQUIRED)
find_package(Tesseract REQUIRED)

if(NOT Tesseract_FOUND)
MESSAGE(FATAL_ERROR "Tesseract was not found. Install Tesseract and recompile OpenCV with Tesseract enabled.")
elseif(HAVE_TESSERACT)
MESSAGE(FATAL_ERROR "Tesseract was found, but OpenCV is not compiled against Tesseract. Recompile OpenCV.")
endif(NOT Tesseract_FOUND)

include_directories("${EASYLOGGINGPP_INCLUDE_DIR}")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../../../common/")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../../../util/")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../")
include_directories("${CMAKE_CURRENT_LIST_DIR}/./")
include_directories(${JsonCpp_INCLUDE_DIR})
include_directories(${Tesseract_INCLUDE_DIR})

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
set(ocrworker_INCLUDES ${dirs} PARENT_SCOPE)

set(ocrworker_LIBS ${EASYLOGGINGPP_LIBRARY} ${JsonCpp_LIBRARY} ${OpenCV_LIBS} Threads::Threads uuid)
set(ocrworker_LIBS ${ocrworker_LIBS} PARENT_SCOPE)

option(BUILD_SHARED_WORKER_LIBS "Build shared libs" OFF)
option(BUILD_OCRWORKER_EXECUTABLE "Build executable of OCRWorker" OFF)

SET(ocrworker_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/imagedata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/postrdata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/base64.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../../common/util.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../worker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../asyncworker.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ocrworker.cpp
    ${EASYLOGGINGPP_INCLUDE_DIR}/easylogging++.cc
)
SET(ocrworker_SRCS "${ocrworker_SRCS}" PARENT_SCOPE)

include(GNUInstallDirs)
if(NOT DEFINED CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
endif()
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_BINDIR "lib" CACHE PATH "user executables (bin)")
endif()

if(BUILD_SHARED_WORKER_LIBS)
    add_library(postr_ocrworker ${ocrworker_SRCS})
    target_compile_definitions(postr_ocrworker PRIVATE -DWORKER_LIBRARY=1) 
    target_link_libraries(postr_ocrworker ${ocrworker_LIBS})
    install(TARGETS postr_ocrworker DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif(BUILD_SHARED_WORKER_LIBS)

if(BUILD_OCRWORKER_EXECUTABLE)
    add_executable(postr_ocr ${ocrworker_SRCS} main.cpp)
    target_link_libraries(postr_ocr ${ocrworker_LIBS})
    install(TARGETS postr_ocr DESTINATION ${CMAKE_INSTALL_BINDIR})
endif(BUILD_OCRWORKER_EXECUTABLE)

install(FILES "${CMAKE_CURRENT_LIST_DIR}/trained_classifierNM1.xml" "${CMAKE_CURRENT_LIST_DIR}/trained_classifierNM2.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/postr/backgroundsegmentation)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/tessdata" DESTINATION ${CMAKE_INSTALL_DATADIR}/postr/ocr)
