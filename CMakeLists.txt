cmake_minimum_required(VERSION 3.16)
project(tortoisegit-revision-graph-crosspaltform VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets)

set(OGDF_WARNING_ERRORS OFF CACHE BOOL "" FORCE)
set(OGDF_INCLUDE_CGAL OFF CACHE BOOL "" FORCE)
set(OGDF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(OGDF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/ogdf EXCLUDE_FROM_ALL)

add_executable(tgit-revision-graph
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/gitgraph.cpp
    src/gitgraph.h
    src/graphview.cpp
    src/graphview.h
    assets/assets.qrc
)

target_link_libraries(tgit-revision-graph PRIVATE Qt5::Widgets OGDF)

if(APPLE)
    set_target_properties(tgit-revision-graph PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "TortoiseGit Revision Graph CrossPaltform"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.tortoisegit.revisiongraph.crosspaltform"
    )
endif()

install(TARGETS tgit-revision-graph RUNTIME DESTINATION bin)
