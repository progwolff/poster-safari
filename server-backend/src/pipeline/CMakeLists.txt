project(Postr)
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR) 

SET(CMAKE_CXX_FLAGS "-std=c++0x")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")

find_package(JsonCpp REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Curlpp REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(EasyLoggingpp)

include_directories("../common/")
include_directories("../extern/")

option(BUILD_SHARED_LIBS "Build shared libs of workers" ON)

include(GNUInstallDirs)
if(NOT DEFINED CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
endif()

find_package(Sanitizers)

option(WITH_OCR "Build OCR worker" ON)
if(WITH_OCR)
    add_subdirectory(workers)
endif(WITH_OCR)

include_directories(${GLOBAL_INCLUDES})

SET(pipeline_SRCS
    main.cpp
    workers/workerloader.cpp
    stream.cpp
    couchdbstream.cpp
    ../common/couchdb.cpp
    ${ocrworker_SRCS}
    ${textgroupcollateworker_SRCS}
    ${regexworker_SRCS}
    ${bgsegmentworker_SRCS}
    ${wordsplitworker_SRCS}
    ${spellcorrectworker_SRCS}
    ${votingtextmergeworker_SRCS}
    ${semanticanalysis_SRCS}
)

add_executable(postersafari ${pipeline_SRCS})
target_link_libraries(postersafari ${EASYLOGGINGPP_LIBRARY} ${JsonCpp_LIBRARY} ${OpenCV_LIBS} ${CURLPP_LIBRARIES} ${GLOBAL_LIBS} dl)
install(TARGETS postersafari DESTINATION ${CMAKE_INSTALL_BINDIR})
add_sanitizers(postersafari)

if(NOT TARGET uninstall)
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
endif(NOT TARGET uninstall)

find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

if(BUILD_DOCUMENTATION)
    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen is needed to build the documentation.")
    else()
        set(doxyfile "${CMAKE_CURRENT_SOURCE_DIR}/../../Doxyfile")

        add_custom_target(doc ALL
            COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.."
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)

        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../html" DESTINATION "${CMAKE_INSTALL_DATADIR}/postr/doc")
    endif()
endif()

INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Image processing pipeline reading information from event posters.")
SET(CPACK_PACKAGE_VENDOR "Moritz Dannehl, Rebecca Lühmann, Nicolas Marin, Michael Nieß, Julian Wolff")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_NAME "postr-processing")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "1")
SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "Postr")
IF(WIN32 AND NOT UNIX)
  SET(CPACK_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}\\\\cmake)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\sc-apps-postr.ico")
  SET(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\sc-apps-postr.ico")
  SET(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}\\\\sc-apps-postr.ico")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME  "bin\\\\postersafari.exe")
  set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut '$desktop\\\\Postr_Processing.lnk' '\$INSTDIR\\\\bin\\\\postersafari.exe'") 
  set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '$desktop\\\\Postr_Processing.lnk'")
  SET(CPACK_NSIS_INSTALLED_ICON_NAME "postersafari.exe")
  SET(CPACK_NSIS_DISPLAY_NAME "Poster Safari")
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\github.com\\\\progwolff\\\\fallstudie")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\github.com\\\\progwolff\\\\fallstudie")
  SET(CPACK_NSIS_CONTACT "fallstudie@julianwolff.de")
  SET(CPACK_NSIS_MODIFY_PATH ON)
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_STRIP_FILES "postersafari")
  SET(CPACK_SOURCE_STRIP_FILES "")
  SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Julian Wolff <fallstudie at julianwolff dot de>")
  SET(CPACK_DEBIAN_PACKAGE_SECTION "misc")
  SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
ENDIF(WIN32 AND NOT UNIX)
SET(CPACK_PACKAGE_EXECUTABLES "postersafari" "Poster Safari")
INCLUDE(CPack)
