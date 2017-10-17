cmake_minimum_required(VERSION 3.0.1)
project(Postr)

set(QT_MIN_VERSION "0.1.0")

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(CMAKE_AUTOUIC ON) 


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")

find_package(JsonCpp REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Curlpp REQUIRED)
find_package(EasyLoggingpp REQUIRED)

find_package(Qt5 ${QT_MIN_VERSION} 
    CONFIG REQUIRED COMPONENTS
    Core
    Concurrent
    Widgets
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories("${CMAKE_CURRENT_LIST_DIR}/../common/")
include_directories("${CMAKE_CURRENT_LIST_DIR}/../extern/")
include_directories(${JsonCpp_INCLUDE_DIR})

set(editor_SRC
    ${CMAKE_CURRENT_LIST_DIR}/../common/imagedata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../common/postrdata.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../common/base64.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../common/util.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../common/couchdb.cpp
    ${EASYLOGGINGPP_INCLUDE_DIR}/easylogging++.cc
    src/main.cpp
    src/editor.cpp
    src/documentmodel.cpp
)

add_executable(postersafari-editor ${editor_SRC})

target_link_libraries(postersafari-editor 
    Qt5::Core
    Qt5::Widgets 
    Qt5::Concurrent
    ${EASYLOGGINGPP_LIBRARY} 
    ${JsonCpp_LIBRARY} 
    ${OpenCV_LIBS} 
    ${CURLPP_LIBRARIES}
    uuid
)

include(GNUInstallDirs)
if(NOT DEFINED CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
endif()
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_BINDIR "lib" CACHE PATH "user executables (bin)")
endif()

# Install the executable
install(TARGETS postersafari-editor DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES postersafari-editor.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
install(FILES sc-apps-postersafari.svg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/icons RENAME postersafari.svg)
